#include <rupa.h>
#include "object.h"

/* ============================================================
 * object.c — new Object(ref?, init?)  (design/object.txt)
 *
 * Object dinamis dengan kemampuan member:
 *   o  = new Object()                 → kosong, tanpa strict
 *   o  = new Object(_, {name:"rudi"}) → `_` = own ref, langsung isi
 *   o1 = new Object(obj, {})          → obj = ref (two-way)
 *   d: People = new Object(_, {name:"rudi"}) → strict kontrak
 *
 * Instance ditandai entry "__object" (boolean). Method member
 * has/get/set/delete/update/json/text TIDAK disimpan sebagai entry —
 * interpretMember (member.c) meng-intercept lewat objectMemberFn()
 * dan men-bind receiver (pola yang sama dengan this.get/this.set).
 *
 * Strict bila layout struct terdaftar:
 *   - stamp "__type" dari kontrak anotasi (statement.c / IR_CHECK
 *     menempelkannya setelah value lolos analyzerCheckStruct), atau
 *   - ref yang sudah distamp.
 *   set/update/delete divalidasi terhadap layout; get/has fallback ke ref.
 *
 * Two-way (sisi object → ref): set/update menulis field yang sudah ada
 * di ref kembali ke binding ref (semSet via "__refname" yang dicatat
 * saat new). Sisi ref → object: get/has membaca ref terkini.
 * ============================================================ */

/* ---------- helpers ---------- */

bool objectIsInstance(RuntimeValue obj) {
  if (obj.type != VALUE_OBJECT) return false;
  RuntimeValue m = valueNull();
  return valueObjectGet(obj, "__object", &m) && m.type == VALUE_BOOLEAN;
}

/* Nama binding ref yang dicatat saat new (NULL bila tidak ada). */
static const char *objectRefName(RuntimeValue obj) {
  if (obj.type != VALUE_OBJECT) return NULL;
  RuntimeValue v = valueNull();
  if (!valueObjectGet(obj, "__refname", &v) || v.type != VALUE_STRING)
    return NULL;
  return v.as.string;
}

/* Nilai ref terkini: binding variabel bila ada, fallback entry __ref. */
static RuntimeValue objectRefValue(RuntimeValue obj, RuntimeEnv *env) {
  RuntimeValue ref = valueNull();
  const char *rn = objectRefName(obj);
  if (rn && env && semGet(env, rn, &ref) && ref.type == VALUE_OBJECT)
    return ref;
  if (env == NULL) { /* tanpa env: pakai snapshot __ref */
    RuntimeValue v = valueNull();
    if (valueObjectGet(obj, "__ref", &v) && v.type == VALUE_OBJECT) return v;
  }
  return valueNull();
}

/* Nama tipe dari arg identifier / literal (tidak dievaluasi bila
 * identifier — sesuai konvensi new T()). Return NULL bila `_`. */
static const char *typeNameArg(Node *node, int id, bool *isUnderscore) {
  *isUnderscore = false;
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) {
    if (a->identifier.name && !strcmp(a->identifier.name, "_")) {
      *isUnderscore = true;
      return NULL;
    }
    return a->identifier.name;
  }
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  return NULL;
}

static const char *strictTypeName(RuntimeValue obj) {
  if (obj.type != VALUE_OBJECT) return NULL;
  RuntimeValue t = valueNull();
  if (!valueObjectGet(obj, "__type", &t) || t.type != VALUE_STRING) return NULL;
  return t.as.string;
}

/* Field kontrak ke-i: false bila index habis / struct tak dikenal. */
static bool strictFieldAt(const char *layout, int index, const char **name,
                          const char **type) {
  return analyzerFieldAt(layout, index, name, type);
}

/* Validasi satu field terhadap layout. Return true bila valid / layout
 * tak dikenal (dinamis). Error ditulis bila gagal. */
static bool strictCheckField(const char *layout, const char *key,
                             RuntimeValue val, Error *error) {
  if (!layout) return true;
  const char *fname = NULL, *ftype = NULL;
  for (int i = 0; strictFieldAt(layout, i, &fname, &ftype); i++) {
    if (!strcmp(fname, key)) {
      if (analyzerCheckType(ftype, val, error)) return true;
      if (error && error->size == 0)
        addRuntimeError(error, ERR_TYPE_MISMATCH, ftype,
                        valueTypeName(val.type));
      return false;
    }
  }
  addRuntimeError(error, ERR_TYPE_MISMATCH, layout, "unknown field");
  return false;
}

/* Key = field kontrak di layout? */
static bool strictIsContractField(const char *layout, const char *key) {
  const char *fname = NULL, *ftype = NULL;
  for (int i = 0; strictFieldAt(layout, i, &fname, &ftype); i++)
    if (!strcmp(fname, key)) return true;
  return false;
}

/* Merge entries src → dst (skip meta "__*" & method). strictCheck:
 * validasi tiap field terhadap layout (bila layout != NULL). */
static bool mergeInto(RuntimeValue dst, RuntimeValue src, const char *layout,
                      Error *error) {
  if (src.type != VALUE_OBJECT) return false;
  for (struct RuntimeObjectEntry *e = src.as.object.entries; e; e = e->next) {
    if (!e->key || !*e->key) continue;              /* anchor implisit */
    if (!strncmp(e->key, "__", 2)) continue;        /* meta */
    if (e->value.type == VALUE_FUNCTION) continue;  /* method */
    if (e->value.type == VALUE_NATIVE_FUNCTION) continue;
    if (layout && !strictCheckField(layout, e->key, e->value, error))
      return false;
    if (!valueObjectSet(&dst, e->key, e->value)) return false;
  }
  return true;
}

/* Hapus entry dengan cara aman-copy: tandai key jadi "" (tersembunyi
 * dari print/get/has/json). Entry kembar bila key di-set lagi akan
 * di-append ke tail shared — terlihat di semua salinan. */
static bool objectEntryDelete(RuntimeValue obj, const char *key) {
  if (obj.type != VALUE_OBJECT || !key) return false;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next) {
    if (e->key && !strcmp(e->key, key)) {
      e->key = gcstrdup("");
      return true;
    }
  }
  return false;
}

/* Sync field yang SUDAH ADA di ref (two-way, object → ref), lalu
 * rebind variabel ref via semSet agar ref kosong pun ikut. */
static void syncBackToRef(RuntimeValue inst, RuntimeEnv *env) {
  RuntimeValue ref = objectRefValue(inst, env);
  if (ref.type != VALUE_OBJECT) return;
  bool changed = false;
  for (struct RuntimeObjectEntry *e = inst.as.object.entries; e; e = e->next) {
    if (!e->key || !*e->key || !strncmp(e->key, "__", 2)) continue;
    if (e->value.type == VALUE_FUNCTION || e->value.type == VALUE_NATIVE_FUNCTION)
      continue;
    RuntimeValue rv;
    if (valueObjectGet(ref, e->key, &rv)) {
      valueObjectSet(&ref, e->key, e->value);
      changed = true;
    }
  }
  if (changed) {
    const char *rn = objectRefName(inst);
    if (rn && env) semSet(env, rn, ref);
  }
}

/* ---------- member methods (argv[0] = receiver instance) ---------- */

static InterpreterResult objHas(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  (void)error;
  if (argc < 2 || argv[1].type != VALUE_STRING || !argv[1].as.string)
    return resultNormal(valueBoolean(false));
  RuntimeValue v = valueNull();
  if (valueObjectGet(argv[0], argv[1].as.string, &v))
    return resultNormal(valueBoolean(true));
  /* fallback: field kontrak yang belum di-set lokal → ada di ref */
  RuntimeValue ref = objectRefValue(argv[0], env);
  if (ref.type == VALUE_OBJECT &&
      valueObjectGet(ref, argv[1].as.string, &v))
    return resultNormal(valueBoolean(true));
  return resultNormal(valueBoolean(false));
}

static InterpreterResult objGet(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  (void)error;
  if (argc < 2 || argv[1].type != VALUE_STRING || !argv[1].as.string)
    return resultNormal(valueNull());
  RuntimeValue v = valueNull();
  if (valueObjectGet(argv[0], argv[1].as.string, &v) && v.type != VALUE_NULL)
    return resultNormal(v);
  RuntimeValue ref = objectRefValue(argv[0], env);
  if (ref.type == VALUE_OBJECT &&
      valueObjectGet(ref, argv[1].as.string, &v) && v.type != VALUE_NULL)
    return resultNormal(v);
  return resultNormal(valueNull());
}

/* set({..}) ATAU set(key, value). Return instance (chainable). */
static InterpreterResult objSet(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  if (argc < 2) return resultNormal(argv[0]);
  const char *layout = strictTypeName(argv[0]);

  if (argv[1].type == VALUE_OBJECT) {
    if (!mergeInto(argv[0], argv[1], layout, error))
      return resultFlow(FLOW_ERROR, valueNull());
  } else if (argc >= 3 && argv[1].type == VALUE_STRING && argv[1].as.string) {
    if (layout && !strictCheckField(layout, argv[1].as.string, argv[2], error))
      return resultFlow(FLOW_ERROR, valueNull());
    if (!valueObjectSet(&argv[0], argv[1].as.string, argv[2]))
      return resultFlow(FLOW_ERROR, valueNull());
  } else {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "set",
                    "expects (key, value) or ({...})");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  syncBackToRef(argv[0], env);
  return resultNormal(argv[0]);
}

static InterpreterResult objDelete(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || argv[1].type != VALUE_STRING || !argv[1].as.string)
    return resultNormal(valueBoolean(false));
  const char *layout = strictTypeName(argv[0]);
  if (layout && strictIsContractField(layout, argv[1].as.string)) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, layout,
                    "cannot delete a contract field");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  bool ok = objectEntryDelete(argv[0], argv[1].as.string);
  return resultNormal(valueBoolean(ok));
}

static InterpreterResult objUpdate(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  if (argc < 2 || argv[1].type != VALUE_OBJECT)
    return resultNormal(argv[0]);
  const char *layout = strictTypeName(argv[0]);
  if (!mergeInto(argv[0], argv[1], layout, error))
    return resultFlow(FLOW_ERROR, valueNull());
  syncBackToRef(argv[0], env);
  return resultNormal(argv[0]);
}

/* ---------- json() / text() ---------- */

static void jsonEmit(RuntimeValue v, char **buf, size_t *cap, size_t *len);

static void jsonPut(char **buf, size_t *cap, size_t *len, const char *s,
                    size_t n) {
  if (*len + n + 1 > *cap) {
    size_t nc = (*len + n + 1) * 2;
    char *tmp = realloc(*buf, nc);
    if (!tmp) return;
    *buf = tmp;
    *cap = nc;
  }
  memcpy(*buf + *len, s, n);
  *len += n;
  (*buf)[*len] = '\0';
}

static void jsonEmitString(const char *s, char **buf, size_t *cap,
                           size_t *len) {
  jsonPut(buf, cap, len, "\"", 1);
  for (const char *p = s ? s : ""; *p; p++) {
    if (*p == '"' || *p == '\\') {
      char esc[2] = {'\\', *p};
      jsonPut(buf, cap, len, esc, 2);
    } else if (*p == '\n') {
      jsonPut(buf, cap, len, "\\n", 2);
    } else if (*p == '\t') {
      jsonPut(buf, cap, len, "\\t", 2);
    } else {
      jsonPut(buf, cap, len, p, 1);
    }
  }
  jsonPut(buf, cap, len, "\"", 1);
}

static void jsonEmit(RuntimeValue v, char **buf, size_t *cap, size_t *len) {
  switch (v.type) {
  case VALUE_NULL:
    jsonPut(buf, cap, len, "null", 4);
    break;
  case VALUE_NUMBER: {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%lld", v.as.number);
    jsonPut(buf, cap, len, tmp, (size_t)n);
    break;
  }
  case VALUE_DECIMAL: {
    char tmp[64];
    int n = snprintf(tmp, sizeof(tmp), "%g", v.as.decimal);
    jsonPut(buf, cap, len, tmp, (size_t)n);
    break;
  }
  case VALUE_BOOLEAN:
    jsonPut(buf, cap, len, v.as.boolean ? "true" : "false", 5);
    break;
  case VALUE_STRING:
    jsonEmitString(v.as.string, buf, cap, len);
    break;
  case VALUE_ARRAY:
    jsonPut(buf, cap, len, "[", 1);
    for (int i = 0; i < v.as.array.length; i++) {
      if (i) jsonPut(buf, cap, len, ",", 1);
      jsonEmit(v.as.array.items[i], buf, cap, len);
    }
    jsonPut(buf, cap, len, "]", 1);
    break;
  case VALUE_OBJECT: {
    jsonPut(buf, cap, len, "{", 1);
    bool first = true;
    for (struct RuntimeObjectEntry *e = v.as.object.entries; e; e = e->next) {
      if (!e->key || !*e->key || !strncmp(e->key, "__", 2)) continue;
      if (e->value.type == VALUE_FUNCTION ||
          e->value.type == VALUE_NATIVE_FUNCTION)
        continue;
      if (!first) jsonPut(buf, cap, len, ",", 1);
      jsonEmitString(e->key, buf, cap, len);
      jsonPut(buf, cap, len, ":", 1);
      jsonEmit(e->value, buf, cap, len);
      first = false;
    }
    jsonPut(buf, cap, len, "}", 1);
    break;
  }
  default:
    jsonPut(buf, cap, len, "null", 4);
    break;
  }
}

static InterpreterResult objJson(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  (void)env;
  (void)argc;
  (void)error;
  size_t cap = 256, len = 0;
  char *buf = malloc(cap);
  if (!buf) return resultFlow(FLOW_ERROR, valueNull());
  buf[0] = '\0';
  jsonEmit(argv[0], &buf, &cap, &len);
  RuntimeValue out = valueString(buf);
  free(buf);
  return resultNormal(out);
}

static InterpreterResult objText(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  (void)env;
  (void)argc;
  (void)error;
  size_t cap = 256, len = 0;
  char *buf = malloc(cap);
  if (!buf) return resultFlow(FLOW_ERROR, valueNull());
  buf[0] = '\0';
  jsonPut(&buf, &cap, &len, "{", 1);
  bool first = true;
  if (argv[0].type == VALUE_OBJECT) {
    for (struct RuntimeObjectEntry *e = argv[0].as.object.entries; e; e = e->next) {
      if (!e->key || !*e->key || !strncmp(e->key, "__", 2)) continue;
      if (e->value.type == VALUE_FUNCTION ||
          e->value.type == VALUE_NATIVE_FUNCTION)
        continue;
      if (!first) jsonPut(&buf, &cap, &len, ", ", 2);
      jsonPut(&buf, &cap, &len, e->key, strlen(e->key));
      jsonPut(&buf, &cap, &len, ": ", 2);
      jsonEmit(e->value, &buf, &cap, &len);
      first = false;
    }
  }
  jsonPut(&buf, &cap, &len, "}", 1);
  RuntimeValue out = valueString(buf);
  free(buf);
  return resultNormal(out);
}

/* ---------- dispatch member → method ---------- */

NativeFn objectMemberFn(const char *name) {
  if (!name) return NULL;
  if (!strcmp(name, "has")) return objHas;
  if (!strcmp(name, "get")) return objGet;
  if (!strcmp(name, "set")) return objSet;
  if (!strcmp(name, "delete")) return objDelete;
  if (!strcmp(name, "update")) return objUpdate;
  if (!strcmp(name, "json")) return objJson;
  if (!strcmp(name, "text")) return objText;
  return NULL;
}

/* Tulis field dari luar (interpretMemberAssign): strict check + sync. */
bool objectMemberWrite(RuntimeValue inst, const char *key, RuntimeValue val,
                       RuntimeEnv *env, Error *error) {
  if (!key || !strncmp(key, "__", 2)) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, key ? key : "?",
                    "internal field is not writable");
    return false;
  }
  const char *layout = strictTypeName(inst);
  if (layout && !strictCheckField(layout, key, val, error)) return false;
  if (!valueObjectSet(&inst, key, val)) return false;
  syncBackToRef(inst, env);
  return true;
}

/* ---------- dispatch member call (raw args) ----------
 * Dipanggil interpretCall SEBELUM evaluasi argumen supaya bentuk
 * design `people.set(name, "anggi")` bisa jalan: identifier arg yang
 * BUKAN variabel terdefinisi dipakai sebagai NAMA FIELD (tidak
 * dievaluasi). Identifier yang terdefinisi (mis. k = "umur") tetap
 * dievaluasi — dynamic key via variabel tetap bisa. */

InterpreterResult objectMemberCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error, bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  if (calleeAst->type != NODE_MEMBER) return resultNormal(valueNull());

  AstNode *m = &node->ast[calleeAst->member.member];
  const char *mname = NULL;
  if (m->type == NODE_IDENTIFIER) mname = m->identifier.name;
  else if (m->type == NODE_LITERAL_ID) mname = m->string.value;
  NativeFn fn = objectMemberFn(mname);
  if (!fn) return resultNormal(valueNull());

  InterpreterResult base = interpretNode(node, calleeAst->member.object, env, error);
  if (base.flow != FLOW_NORMAL) return base;
  if (!objectIsInstance(base.value)) return resultNormal(valueNull());

  *handled = true;
  int argc = 1 + ast->call.length;
  RuntimeValue *argv = calloc((size_t)(argc > 0 ? argc : 1), sizeof(RuntimeValue));
  if (!argv) {
    addRuntimeError(error, ERR_INTERNAL, "object", "out of memory");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  argv[0] = base.value;
  for (int i = 0; i < ast->call.length; i++) {
    int argId = ast->call.args[i];
    if (argId < 0 || argId >= node->length) { argv[i + 1] = valueNull(); continue; }
    AstNode *argAst = &node->ast[argId];
    /* Identifier di rupa bisa NODE_IDENTIFIER atau NODE_LITERAL_ID. */
    const char *ident = NULL;
    if (argAst->type == NODE_IDENTIFIER) ident = argAst->identifier.name;
    else if (argAst->type == NODE_LITERAL_ID) ident = argAst->string.value;
    if (i == 0 && ident && strcmp(ident, "this") != 0) {
      /* field-name sugar HANYA di posisi key (arg pertama): identifier
       * bukan variabel terdefinisi = nama field (design:
       * `people.set(name, "anggi")`). Posisi value dievaluasi normal. */
      RuntimeValue probe = valueNull();
      if (!semGet(env, ident, &probe)) {
        argv[i + 1] = valueString(ident);
        continue;
      }
    }
    InterpreterResult ar = interpretNode(node, argId, env, error);
    if (ar.flow != FLOW_NORMAL) { free(argv); return ar; }
    argv[i + 1] = ar.value;
  }
  InterpreterResult result = fn(argc, argv, env, error);
  free(argv);
  return result;
}

/* ---------- hook new Object(...) ---------- */

InterpreterResult objectNewCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error, bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL || !env)
    return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  const char *name = NULL;
  if (calleeAst->type == NODE_IDENTIFIER)
    name = calleeAst->identifier.name;
  else if (calleeAst->type == NODE_LITERAL_ID)
    name = calleeAst->string.value;
  if (!name || strcmp(name, "new") != 0) return resultNormal(valueNull());

  /* args[0] harus literal "Object" (nama tipe tak dievaluasi). */
  if (ast->call.length < 1 || ast->call.args[0] < 0 ||
      ast->call.args[0] >= node->length)
    return resultNormal(valueNull());
  AstNode *typeAst = &node->ast[ast->call.args[0]];
  const char *typeName = NULL;
  if (typeAst->type == NODE_IDENTIFIER)
    typeName = typeAst->identifier.name;
  else if (typeAst->type == NODE_LITERAL_ID)
    typeName = typeAst->string.value;
  if (!typeName || strcmp(typeName, "Object") != 0)
    return resultNormal(valueNull());

  *handled = true;

  bool refIsUnderscore = false;
  const char *refIdent = ast->call.length >= 2
                             ? typeNameArg(node, ast->call.args[1],
                                           &refIsUnderscore)
                             : NULL;
  bool hasInit = ast->call.length >= 3 && ast->call.args[2] >= 0;

  RuntimeValue inst = valueObject(NULL);
  valueObjectSet(&inst, "__object", valueBoolean(true));

  /* ref: identifier non-underscore = binding yang dijadikan referensi. */
  if (ast->call.length >= 2 && !refIsUnderscore && refIdent) {
    RuntimeValue probe = valueNull();
    if (semGet(env, refIdent, &probe)) {
      valueObjectSet(&inst, "__ref", probe);
      valueObjectSet(&inst, "__refname", valueString(refIdent));
    }
  }

  /* strict: ref yang sudah distamp, ATAU nama struct terdaftar
   * (bentuk new Object(People, {...}) — ref jadi nama layout). */
  RuntimeValue refVal = objectRefValue(inst, env);
  const char *strictFrom = strictTypeName(refVal);
  if (!strictFrom && refIdent && !refIsUnderscore &&
      analyzerFindStruct(refIdent))
    strictFrom = refIdent;
  if (strictFrom) valueObjectSet(&inst, "__type", valueString(strictFrom));

  /* isi awal */
  if (hasInit) {
    InterpreterResult init = interpretNode(node, ast->call.args[2], env, error);
    if (init.flow != FLOW_NORMAL) return init;
    if (init.value.type == VALUE_OBJECT) {
      if (!mergeInto(inst, init.value, strictFrom, error))
        return resultFlow(FLOW_ERROR, valueNull());
    } else if (init.value.type != VALUE_NULL) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "Object",
                      "init expects an object literal");
      return resultFlow(FLOW_ERROR, valueNull());
    }
  }

  return resultNormal(inst);
}
