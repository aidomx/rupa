#include <rupa.h>

/* Nama parameter dari AST param node (IDENTIFIER / ANNOTATION). */
static const char *paramName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  if (ast->type == NODE_ANNOTATION) {
    int name = ast->annotation.name;
    if (name >= 0 && name < node->length && node->ast[name].type == NODE_IDENTIFIER)
      return node->ast[name].identifier.name;
  }
  return NULL;
}

/* Body struct direpresentasikan sebagai block berisi NODE_ANNOTATION
 * (name: type). Ekstrak pasangan (nama, tipe) dari block itu. */
static int extractFields(Node *node, int bodyId, struct StructField **out) {
  *out = NULL;
  if (!node || bodyId < 0 || bodyId >= node->length) return 0;

  AstNode *body = &node->ast[bodyId];
  if (body->type != NODE_BLOCK) return 0;

  int count = 0;
  struct StructField *fields = gcmall(sizeof(*fields) * (size_t)body->block.length);
  if (!fields) return 0;

  for (int i = 0; i < body->block.length; i++) {
    int sid = body->block.statements[i];
    if (sid < 0 || sid >= node->length) continue;

    AstNode *s = &node->ast[sid];
    if (s->type != NODE_ANNOTATION || s->annotation.value >= 0) continue;

    const char *fname = NULL;
    AstNode *fn = &node->ast[s->annotation.name];
    if (fn->type == NODE_IDENTIFIER)
      fname = fn->identifier.name;
    else if (fn->type == NODE_LITERAL_ID)
      fname = fn->string.value;
    if (!fname) continue;

    /* Tipe field sebagai string ("number", "Circuit[]", ...). */
    char typeName[256];
    if (!formatAstTypeName(node, s->annotation.type, typeName, sizeof(typeName)))
      continue;

    fields[count].name = gcstrdup(fname);
    fields[count].type = gcstrdup(typeName);
    count++;
  }

  *out = fields;
  return count;
}

/* =====================================================================
 * Enum runtime (design/enum.txt).
 *
 * Enum declaration meng-bind konstanta ke environment:
 *   enum Color { RED, GREEN = 5, BLUE, NAME: string = "red" }
 *   - member tanpa nilai → auto-increment (0, 1, ...) dari counter
 *     yang berlanjut setelah member bernilai eksplisit.
 *   - member bertipe + nilai → nilai dievaluasi (number/string/bool).
 * Nama enum itu sendiri juga di-bind sebagai object berisi seluruh
 * member (plus marker "__enum" = nama enum), sehingga `Color.RED` bisa
 * dibaca lewat member access dan member tak dikenal ditolak eksplisit.
 * ===================================================================== */
static bool enumMemberName(Node *node, int id, const char **out) {
  if (id < 0 || id >= node->length) return false;
  AstNode *m = &node->ast[id];
  /* Member bentuk baru (grammar_enum.c): Annotation(Name, Type, Value)
   * atau Identifier bare (auto-increment). */
  if (m->type == NODE_ANNOTATION && m->annotation.name >= 0) {
    AstNode *t = &node->ast[m->annotation.name];
    if (t->type == NODE_IDENTIFIER) {
      *out = t->identifier.name;
      return true;
    }
    if (t->type == NODE_LITERAL_ID) {
      *out = t->string.value;
      return true;
    }
  }
  if (m->type == NODE_IDENTIFIER) {
    *out = m->identifier.name;
    return true;
  }
  if (m->type == NODE_LITERAL_ID) {
    *out = m->string.value;
    return true;
  }
  return false;
}

static void enumAppendEntry(struct RuntimeObjectEntry **entries, const char *key,
                            RuntimeValue value) {
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  if (!e) return;
  e->key = gcstrdup(key);
  e->value = value;
  e->next = NULL;
  if (!*entries) {
    *entries = e;
    return;
  }
  struct RuntimeObjectEntry *tail = *entries;
  while (tail->next)
    tail = tail->next;
  tail->next = e;
}

InterpreterResult interpretEnum(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error) {
  if (!node || !ast || ast->type != NODE_ENUM_DECL)
    return resultNormal(valueNull());

  const char *name = NULL;
  if (ast->asEnum.name >= 0 && ast->asEnum.name < node->length) {
    AstNode *n = &node->ast[ast->asEnum.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  long long next = 0; /* counter auto-increment member tanpa nilai */
  struct RuntimeObjectEntry *entries = NULL;

  if (ast->asEnum.body >= 0 && ast->asEnum.body < node->length) {
    AstNode *body = &node->ast[ast->asEnum.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        int sid = body->block.statements[i];
        if (sid < 0 || sid >= node->length) continue;
        AstNode *m = &node->ast[sid];

        const char *mname = NULL;
        if (!enumMemberName(node, sid, &mname)) continue;

        /* Value: NODE_ANNOTATION membawa .value (-1 = tanpa nilai →
         * auto). Nilai eksplisit dievaluasi sebagai expression biasa
         * (number, string, bool, ...). */
        int valueId = -1;
        if (m->type == NODE_ANNOTATION)
          valueId = m->annotation.value;

        if (valueId >= 0) {
          InterpreterResult r = interpretNode(node, valueId, env, error);
          if (r.flow == FLOW_ERROR) return r;
          if (r.value.type == VALUE_NUMBER) {
            next = r.value.as.number + 1;
          } else {
            next = 0; /* non-number: counter kembali ke 0 untuk member berikutnya */
          }
          semSet(env, mname, r.value);
          enumAppendEntry(&entries, mname, r.value);
        } else {
          /* Member tanpa nilai eksplisit → konstanta numerik auto. */
          RuntimeValue v = valueNumber(next);
          semSet(env, mname, v);
          enumAppendEntry(&entries, mname, v);
          next++;
        }
      }
    }
  }

  if (name) {
    semDeclare(env, name, "enum");
    /* Marker metadata enum: dipakai interpretMember/interpretMemberAssign
     * untuk error eksplisit (member tak dikenal / member write) yang
     * menyebut nama enum. Value = nama enum (VALUE_STRING). Marker
     * disembunyikan dari print, equality, dan salinan modul. */
    RuntimeValue enumVal = valueObject(entries);
    valueObjectSet(&enumVal, "__enum", valueString(name));
    semSet(env, name, enumVal);
  }

  return resultNormal(valueNull());
}

InterpreterResult interpretStruct(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  (void)error;
  if (!node || !ast || ast->type != NODE_STRUCT_DECL)
    return resultNormal(valueNull());

  /* Struct declaration registers a type name in the environment.
   * The actual field layout is described by the body block's annotations,
   * but at runtime we only need to know the type exists so that
   * annotation validation can accept it. */
  const char *name = NULL;
  if (ast->asStruct.name >= 0 && ast->asStruct.name < node->length) {
    AstNode *n = &node->ast[ast->asStruct.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  if (name) {
    semDeclare(env, name, "struct");

    /* Daftarkan layout field ke analyzer — kontrak untuk validasi
     * annotation struct-first (design/next_struct.txt). */
    struct StructField *fields = NULL;
    int count = extractFields(node, ast->asStruct.body, &fields);
    analyzerDeclareStruct(name, fields, count);

    /* Referensi type tak dikenal di field (typo, belum dideklarasi)
     * ditolak — dilakukan SETELAH deklarasi agar self-reference
     * (`Node { next: Node[] }`) dan urutan bebas tetap valid. */
    for (int i = 0; i < count; i++) {
      if (!analyzerIsKnownType(fields[i].type)) {
        if (error) {
          setRuntimeErrorLocation(0, 0);
          char buffer[256];
          snprintf(buffer, sizeof(buffer), "unknown type '%s' for field %s",
                   fields[i].type, fields[i].name);
          addRuntimeError(error, ERR_TYPE_MISMATCH, name, buffer);
        }
        return resultFlow(FLOW_ERROR, valueNull());
      }
    }
  }

  return resultNormal(valueNull());
}

/* Class (design/new_class.txt): registrasi runtime sama dengan struct —
 * type name dikenal analyzer, layout = field di body. Method (function
 * decl) dalam body tidak dieksekusi di sini; binding terjadi saat dipakai. */
/* =====================================================================
 * Class runtime (design/new_class.txt langkah 2: this/closure).
 *
 * FILOSOFI: object TIDAK di-instantiate pada deklarasi — instance
 * "default" SUDAH ADA sejak class dideklarasikan, di-bind ke nama class.
 * interpretClass membangun RuntimeObject instance:
 *   - method di body → VALUE_FUNCTION (closure env deklarasi) disimpan
 *     sebagai entry object, dikenali member call `Monster.method()`.
 *   - field annotation di body → field instance (nilai awal null).
 * Method menerima `this` = instance object tsb (di-bind di call frame
 * oleh interpretCall). Instantiation `c = Counter({...})` (opsional)
 * membuat instance BARU dengan construct(args) — lihat interpretCall.
 * ===================================================================== */
static RuntimeValue classBuildInstance(Node *node, AstNode *ast, RuntimeEnv *env);

/* Panggil method construct() otomatis — constructor ditetapkan
 * (design/new_class.txt poin 3). `this` = instance. args optional:
 * dipakai saat instantiation `Counter({value: 1})` (args array object);
 * tanpa args (deklarasi class), construct tetap dipanggil tanpa argumen. */
void classRunConstruct(Node *node, RuntimeValue instance, RuntimeEnv *env,
                       Error *error, RuntimeValue args) {
  classRunConstructExt(node, instance, env, error, args, true);
}

/* Varian dengan kontrol @input: allowInput=false dipakai auto-construct
 * deklarasi class — construct yang butuh `input` (param Input) TIDAK
 * dijalankan saat deklarasi (input hanya aktif saat instantiation/
 * program jalan). */
void classRunConstructExt(Node *node, RuntimeValue instance, RuntimeEnv *env,
                          Error *error, RuntimeValue args, bool allowInput) {
  (void)node;
  RuntimeValue ctor;
  if (!valueObjectGet(instance, "construct", &ctor) ||
      ctor.type != VALUE_FUNCTION || !ctor.as.function)
    return;

  if (!allowInput && ctor.as.function->paramLength > 0 &&
      ctor.as.function->params) {
    for (int pi = 0; pi < ctor.as.function->paramLength && pi < 8; pi++) {
      const char *pn = paramName(ctor.as.function->node,
                                 ctor.as.function->params[pi]);
      if (pn && !strcmp(pn, "input"))
        return; /* butuh input program — jangan jalan saat deklarasi. */
    }
  }

  RuntimeFunction *fn = ctor.as.function;
  RuntimeEnv *local = semCreateEnv(fn->closure ? fn->closure : env);
  if (!local) return;
  semSet(local, "this", instance);

  /* @input (design/new_class.txt poin 5): construct dengan param
   * `input` menerima object Input AKTIF hanya bila class punya handler
   * @input — evaluasi handler di sini (return value = spec field:
   * prompt), lalu bind object Input ke param. Tanpa handler @input,
   * `input.get()` ReferenceError (inputSpecActive() false). */
  RuntimeValue inputfn = valueNull();
  if (valueObjectGet(instance, "_inputfn", &inputfn) &&
      inputfn.type == VALUE_FUNCTION && inputfn.as.function) {
    RuntimeFunction *hf = inputfn.as.function;
    RuntimeEnv *hlocal = semCreateEnv(hf->closure ? hf->closure : env);
    if (hlocal) {
      semSet(hlocal, "this", instance);
      InterpreterResult hr =
          interpretNode(hf->node, hf->body, hlocal, error);
      if (hr.flow == FLOW_RETURN || hr.flow == FLOW_NORMAL)
        inputSpecRegister(hr.value);
    }
  }
  if (fn->paramLength > 0 && fn->params) {
    for (int pi = 0; pi < fn->paramLength && pi < 8; pi++) {
      const char *pname = paramName(fn->node, fn->params[pi]);
      if (pname && !strcmp(pname, "input"))
        semSet(local, pname, inputCreateObject());
    }
  }

  /* Bind param pertama (mis. `args: unknown[]`) bila construct punya
   * parameter. SELALU di-bind: instantiation meneruskan args dari call;
   * construct otomatis dari deklarasi class mendapat array kosong
   * (args.length == 0) — bukan ReferenceError. Param `input` di-skip:
   * sudah di-bind object Input di atas (atau sengaja tak di-bind). */
  if (fn->paramLength > 0 && fn->params) {
    const char *pname = paramName(fn->node, fn->params[0]);
    if (pname && strcmp(pname, "input") != 0)
      semSet(local, pname,
             args.type == VALUE_NULL ? valueArray(NULL, 0) : args);
  }

  interpretNode(fn->node, fn->body, local, error);
}

InterpreterResult interpretClass(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  if (!node || !ast || ast->type != NODE_CLASS_DECL)
    return resultNormal(valueNull());

  const char *name = NULL;
  if (ast->asClass.name >= 0 && ast->asClass.name < node->length) {
    AstNode *n = &node->ast[ast->asClass.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  if (name) {
    const char *parentName = NULL;
    if (ast->asClass.parent >= 0 && ast->asClass.parent < node->length) {
      AstNode *pn = &node->ast[ast->asClass.parent];
      if (pn->type == NODE_IDENTIFIER)
        parentName = pn->identifier.name;
      else if (pn->type == NODE_LITERAL_ID)
        parentName = pn->string.value;
      else if (pn->type == NODE_ARRAY_TYPE && pn->arrayType.elementType >= 0)
        parentName = NULL;
    }

    semDeclare(env, name, "class");

    /* Layout field dari body (annotation tanpa value) — declareClass:
     * menandai sebagai class + parent (extends) untuk super dispatch. */
    struct StructField *fields = NULL;
    int count = extractFields(node, ast->asClass.body, &fields);
    analyzerDeclareClassExt(name, fields, count, parentName);

    /* Instance object (langkah 2 this/closure): object sudah ada
     * sejak deklarasi — tanpa `new`, tanpa instantiation. */
    RuntimeValue instance = classBuildInstance(node, ast, env);
    if (instance.type == VALUE_OBJECT) {
      semSet(env, name, instance);
      /* construct() otomatis — memanggil sebelum binding supaya mutasi
       * field via append (valueObjectSet) tetap terlihat di instance.
       * allowInput=false: construct ber-param `input` tidak jalan saat
       * deklarasi (belum ada program yang memakai class ini). */
      classRunConstructExt(node, instance, env, error, valueNull(), false);
    }
  }

  return resultNormal(valueNull());
}

/* Bangun instance object dari body class:
 * - NODE_FUNCTION_DECL di body → method (VALUE_FUNCTION, closure env
 *   deklarasi) sebagai entry object.
 * - NODE_ANNOTATION tanpa value → field instance (null).
 * Entry APPEND (bukan prepend) supaya urutan stabil dan mutasi via
 * valueObjectSet terlihat di semua salinan RuntimeValue. */
static void classAppendEntry(struct RuntimeObjectEntry **entries, const char *key,
                             RuntimeValue value) {
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  if (!e) return;
  e->key = gcstrdup(key);
  e->value = value;
  e->next = NULL;
  if (!*entries) {
    *entries = e;
    return;
  }
  struct RuntimeObjectEntry *tail = *entries;
  while (tail->next)
    tail = tail->next;
  tail->next = e;
}

static RuntimeValue classBuildInstance(Node *node, AstNode *ast, RuntimeEnv *env) {
  struct RuntimeObjectEntry *entries = NULL;

  const char *createdName = NULL;
  const char *selfName = NULL;
  int inputHandlerId = -1; /* body node method ber-marker @input */
  if (ast->asClass.name >= 0 && ast->asClass.name < node->length) {
    AstNode *n = &node->ast[ast->asClass.name];
    if (n->type == NODE_IDENTIFIER)
      selfName = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      selfName = n->string.value;
  }

  if (ast->asClass.body >= 0 && ast->asClass.body < node->length) {
    AstNode *body = &node->ast[ast->asClass.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        int sid = body->block.statements[i];
        if (sid < 0 || sid >= node->length) continue;
        AstNode *s = &node->ast[sid];

        if (s->type == NODE_FUNCTION_DECL) {
          /* Method: closure = env deklarasi class (sejajar interpretFunction). */
          InterpreterResult fr = interpretFunction(node, s, env, NULL);
          if (fr.value.type != VALUE_FUNCTION) continue;
          AstNode *fn = &node->ast[s->function.name];
          const char *mname =
              fn && fn->type == NODE_IDENTIFIER     ? fn->identifier.name
              : (fn && fn->type == NODE_LITERAL_ID) ? fn->string.value
                                                    : "anonymous";
          classAppendEntry(&entries, mname, fr.value);
        } else if (s->type == NODE_MARKER && s->asClass.body >= 0) {
          /* @created (design/new_class.txt poin 4): method ber-marker —
           * method tetap di-build sebagai entry biasa; body marker
           * (function decl) di-telusuri. Registry created diisi oleh
           * interpretClass (butuh nama class). */
          AstNode *inner = &node->ast[s->asClass.body];
          if (inner->type == NODE_FUNCTION_DECL) {
            InterpreterResult fr = interpretFunction(node, inner, env, NULL);
            if (fr.value.type == VALUE_FUNCTION) {
              AstNode *fn = &node->ast[inner->function.name];
              const char *mname =
                  fn && fn->type == NODE_IDENTIFIER     ? fn->identifier.name
                  : (fn && fn->type == NODE_LITERAL_ID) ? fn->string.value
                                                        : "anonymous";
              classAppendEntry(&entries, mname, fr.value);
              /* Marker @created: catat nama method untuk lifecycle. */
              if (s->asClass.name >= 0 && s->asClass.name < node->length) {
                AstNode *mn = &node->ast[s->asClass.name];
                const char *mark = mn->type == NODE_IDENTIFIER   ? mn->identifier.name
                                   : mn->type == NODE_LITERAL_ID ? mn->string.value
                                                                 : NULL;
                if (mark && !strcmp(mark, "created"))
                  createdName = mname;
              }
              /* Marker @input: catat method handler (design poin 5). */
              if (s->asClass.name >= 0 && s->asClass.name < node->length) {
                AstNode *mn2 = &node->ast[s->asClass.name];
                const char *mark2 =
                    mn2->type == NODE_IDENTIFIER     ? mn2->identifier.name
                    : mn2->type == NODE_LITERAL_ID ? mn2->string.value
                                                   : NULL;
                if (mark2 && !strcmp(mark2, "input"))
                  inputHandlerId = s->asClass.body;
              }
            }
          }
        } else if (s->type == NODE_ANNOTATION && s->annotation.value < 0) {
          /* Field instance: nilai awal null. */
          AstNode *fn = &node->ast[s->annotation.name];
          const char *fname =
              fn && fn->type == NODE_IDENTIFIER     ? fn->identifier.name
              : (fn && fn->type == NODE_LITERAL_ID) ? fn->string.value
                                                    : "field";
          classAppendEntry(&entries, fname, valueNull());
        }
      }
    }
  }

  /* extends (design/new_class.txt): method induk yang TIDAK dioverride
   * diwarisi ke instance (orientasi presentation). Method anak menang.
   * super dispatch tetap bisa menembus ke prototype induk via
   * analyzerClassParent. */
  if (selfName) {
    const char *pn = analyzerClassParent(selfName);
    for (int depth = 0; pn && depth < 16; depth++) {
      RuntimeValue pinst = valueNull();
      if (!semGet(env, pn, &pinst) || pinst.type != VALUE_OBJECT) break;
      for (struct RuntimeObjectEntry *e = pinst.as.object.entries; e; e = e->next) {
        /* @created induk terwarisi bila anak tidak punya marker sendiri. */
        if (e->key && !strcmp(e->key, "_created") && !createdName) {
          if (e->value.type == VALUE_STRING)
            createdName = e->value.as.string;
          continue;
        }
        if (e->value.type != VALUE_FUNCTION) continue;
        bool overridden = false;
        for (struct RuntimeObjectEntry *m = entries; m; m = m->next) {
          if (!strcmp(m->key, e->key)) {
            overridden = true;
            break;
          }
        }
        if (!overridden)
          classAppendEntry(&entries, e->key, e->value);
      }
      pn = analyzerClassParent(pn);
    }
  }

  /* Registry internal instance: nama class + method @created + handler
   * @input (bila ada). */
  if (selfName)
    classAppendEntry(&entries, "_class", valueString(selfName));
  if (createdName)
    classAppendEntry(&entries, "_created", valueString(createdName));
  if (inputHandlerId >= 0) {
    InterpreterResult fr =
        interpretFunction(node, &node->ast[inputHandlerId], env, NULL);
    if (fr.value.type == VALUE_FUNCTION)
      classAppendEntry(&entries, "_inputfn", fr.value);
  }

  return valueObject(entries);
}

/* Lifecycle @created (design/new_class.txt poin 4): method ber-marker
 * @created dipanggil saat object "dipakai" — untuk sekarang: saat
 * instantiation `new ClassName(args)` (instance baru), di classInstance.
 * Instance default (deklarasi class) TIDAK ikut — object itu sudah ada
 * sejak deklarasi, bukan hasil pemakaian. */
void classRunLifecycle(Node *node, RuntimeValue instance, RuntimeEnv *env,
                       Error *error, RuntimeValue args) {
  RuntimeValue created;
  if (!valueObjectGet(instance, "_created", &created) ||
      created.type != VALUE_STRING || !created.as.string)
    return;
  RuntimeValue method;
  if (!valueObjectGet(instance, created.as.string, &method) ||
      method.type != VALUE_FUNCTION || !method.as.function)
    return;

  RuntimeFunction *fn = method.as.function;
  RuntimeEnv *local = semCreateEnv(fn->closure ? fn->closure : env);
  if (!local) return;
  semSet(local, "this", instance);
  if (fn->paramLength > 0 && fn->params) {
    const char *pname = paramName(fn->node, fn->params[0]);
    if (pname)
      semSet(local, pname,
             args.type == VALUE_NULL ? valueArray(NULL, 0) : args);
  }
  interpretNode(fn->node, fn->body, local, error);
}
