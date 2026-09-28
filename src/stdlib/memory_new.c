#include <rupa.h>

/* memory_new.c — sistem memori type-driven: new (design/new_memory.txt).
 *
 * Layer di atas gc.c yang MEMANFAATKAN pengetahuan tipe:
 *   new Number()        — 1 elemen, ukuran otomatis sizeof(number) (zeroed)
 *   new Number(n)       — n elemen, zeroed
 *   new Number(src, n)  — realloc ke n elemen (handle lama dangling)
 *
 * Kapitalisasi PENTING: bentuknya `new Number()`, bukan `new number()`
 * — `new number()` bentrok dengan `x: number` (penanda tipe). Argument
 * new SELALU nama tipe (Number/String/Struct), tidak pernah dievaluasi.
 *
 * `n` = jumlah ELEMEN, bukan byte — byte tidak pernah muncul di API.
 * Zeroed (calloc-style): tidak ada state uninitialized.
 *
 * Unit lain hasil split memory.c:
 *   memory_index.c   — indexing VALUE_PTR (get/set interpreter+IR)
 *   memory_member.c  — member access struct + string slot
 *   memory_builtin.c — del/dupl/compare + memoryInit
 */

/* ==================== new ==================== */

/* Nama tipe dari node argumen `new`: identifier/literal, bentuk `T[]`
 * (NODE_SUBSCRIPT kosong di konteks ekspresi), atau NODE_ARRAY_TYPE. */
static bool newTypeArgName(Node *node, int id, char *buffer, size_t capacity) {
  if (!node || id < 0 || id >= node->length || !buffer || capacity == 0) return false;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) {
    int written = snprintf(buffer, capacity, "%s", a->identifier.name);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type == NODE_LITERAL_ID) {
    int written = snprintf(buffer, capacity, "%s", a->string.value);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type == NODE_SUBSCRIPT && a->subscript.index < 0) {
    AstNode *base = &node->ast[a->subscript.posId];
    const char *name = base->type == NODE_IDENTIFIER   ? base->identifier.name
                       : base->type == NODE_LITERAL_ID ? base->string.value
                                                       : NULL;
    if (!name) return false;
    int written = snprintf(buffer, capacity, "%s[]", name);
    return written > 0 && (size_t)written < capacity;
  }
  return formatAstTypeName(node, id, buffer, capacity);
}

/* Normalisasi nama tipe bentuk type-driven:
 *   "Number" -> "number", "String" -> "string", "Boolean" -> "boolean",
 *   "Decimal" -> "decimal", "Ptr" -> "ptr", lainnya verbatim
 * (struct pakai nama asli: "People", "Monster"). */
static void newTypeNormalize(const char *in, char *out, size_t capacity) {
  static const struct {
    const char *typeName;
    const char *alias;
  } aliases[] = {
      {"Number", "number"},   {"String", "string"}, {"Boolean", "boolean"},
      {"Decimal", "decimal"}, {"Ptr", "ptr"},
  };
  for (size_t i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
    if (!strcmp(in, aliases[i].typeName)) {
      snprintf(out, capacity, "%s", aliases[i].alias);
      return;
    }
  }
  snprintf(out, capacity, "%s", in);
}

/* Arg 0 (src) dievaluasi jadi nilai primitif untuk handle scalar
 * (rebinding `p = 7` menulis nilai, bukan ptr) — deref via binding
 * bila arg berbentuk identifier. Return true bila *out = VALUE_PTR. */
static bool newPtrArgDeref(Node *node, int argId, RuntimeEnv *env, RuntimeValue *out) {
  if (!node || argId < 0 || argId >= node->length) return false;
  AstNode *arg = &node->ast[argId];
  const char *nm = arg->type == NODE_IDENTIFIER   ? arg->identifier.name
                   : arg->type == NODE_LITERAL_ID ? arg->string.value
                                                  : NULL;
  if (!nm) return false;
  RuntimeValue v = valueNull();
  if (!semGet(env, nm, &v)) return false;
  if (v.type != VALUE_PTR || !v.as.ptr) return false;
  *out = v;
  return true;
}

InterpreterResult memoryNewCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error,
                                bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  const char *name = NULL;
  if (calleeAst->type == NODE_IDENTIFIER)
    name = calleeAst->identifier.name;
  else if (calleeAst->type == NODE_LITERAL_ID)
    name = calleeAst->string.value;
  if (!name || strcmp(name, "new") != 0) return resultNormal(valueNull());

  *handled = true;
  if (ast->call.length < 1 || ast->call.args[0] < 0 || ast->call.args[0] >= node->length)
    return resultNormal(valueNull());

  /* Arg[0] = nama tipe, tidak dievaluasi. */
  char typeRaw[256];
  if (!newTypeArgName(node, ast->call.args[0], typeRaw, sizeof(typeRaw)))
    return resultNormal(valueNull());
  char type[256];
  newTypeNormalize(typeRaw, type, sizeof(type));

  int elemSize = 0;
  bool typed = rupaMemorySizeOf(type, &elemSize) && elemSize > 0;
  bool isContract = !strcmp(type, "Contract");
  bool isRecontract = !strcmp(type, "Recontract");

  /* new Recontract(src, n) — realloc (design/new_memory.txt tabel:
   * padanan repin). elemSize diambil dari registry blok src, bukan
   * dari tipe — Recontract bukan tipe sesungguhnya. Handle lama
   * dangling (realloc selalu pindah); type elemen src diturunkan. */
  if (isRecontract) {
    if (ast->call.length != 3) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Recontract(src, n)",
                      "expects exactly (src, count)");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    InterpreterResult r1 = interpretNode(node, ast->call.args[1], env, error);
    if (r1.flow != FLOW_NORMAL) return r1;
    InterpreterResult r2 = interpretNode(node, ast->call.args[2], env, error);
    if (r2.flow != FLOW_NORMAL) return r2;

    RuntimeValue src = r1.value;
    if ((src.type != VALUE_PTR || !src.as.ptr) &&
        !newPtrArgDeref(node, ast->call.args[1], env, &src)) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Recontract(src, n)", "src must be a ptr");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (r2.value.type != VALUE_NUMBER || r2.value.as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Recontract(src, n)",
                      "count must be a positive number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (gcregisview(src.as.ptr)) {
      addRuntimeError(error, ERR_MEMORY, "new Recontract(src, n)",
                      "src is a view into a struct block — realloc the owner instead");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (gcfind(src.as.ptr) < 0) {
      addRuntimeError(error, ERR_MEMORY, "new Recontract(src, n)", "pointer not owned by GC");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    {
      size_t se = gcelem(src.as.ptr);
      size_t ss = gcsize(src.as.ptr);
      int esz = (se && se != GC_VIEW_MAGIC && ss) ? (int)(ss / se) : 0;
      if (esz <= 0) {
        addRuntimeError(error, ERR_MEMORY, "new Recontract(src, n)",
                        "cannot determine element size of src");
        return resultFlow(FLOW_ERROR, valueNull());
      }
      size_t count = (size_t)r2.value.as.number;
      void *handle = gcarray(src.as.ptr, count, (size_t)esz);
      if (!handle) {
        addRuntimeError(error, ERR_INTERNAL, "new Recontract(src, n)", "out of memory");
        return resultFlow(FLOW_ERROR, valueNull());
      }
      const char *st = gcregtype(src.as.ptr);
      if (st) gcregsettype(handle, st);
      return resultNormal(valuePtr(handle));
    }
  }

  /* new Contract(count, elemsize) — calloc custom: ukuran elemen
   * eksplisit dari argumen, bukan dari tipe anotasi (blok mentah
   * ber-elemen kecil, mis. byte). Tanpa registrasi tipe elemen. */
  if (isContract && ast->call.length == 3) {
    InterpreterResult r1 = interpretNode(node, ast->call.args[1], env, error);
    if (r1.flow != FLOW_NORMAL) return r1;
    InterpreterResult r2 = interpretNode(node, ast->call.args[2], env, error);
    if (r2.flow != FLOW_NORMAL) return r2;
    if (r1.value.type != VALUE_NUMBER || r1.value.as.number <= 0 ||
        r2.value.type != VALUE_NUMBER || r2.value.as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract(count, elemsize)",
                      "count and elemsize must be positive numbers");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    void *handle =
        gccalloc((size_t)r1.value.as.number, (size_t)r2.value.as.number);
    if (!handle) {
      addRuntimeError(error, ERR_INTERNAL, "new Contract(count, elemsize)", "out of memory");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    return resultNormal(valuePtr(handle));
  }

  if (!typed) {
    if (isContract) {
      /* C1: Contract tanpa anotasi (polos) — kontraknya anotasi itu. */
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract()",
                      "requires a type annotation (e.g. p: number = new Contract())");
    } else {
      char message[512];
      snprintf(message, sizeof(message), "unknown type '%s' in new T()", type);
      addRuntimeError(error, ERR_UNDEFINED_VAR, type, message);
    }
    return resultFlow(FLOW_ERROR, valueNull());
  }

  /* args[1..] = angka: [n] atau [src, n]. */
  RuntimeValue vals[2];
  int nvals = 0;
  for (int i = 1; i < ast->call.length; i++) {
    if (nvals >= 2) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T()", "expects at most 2 numbers (src, n)");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    InterpreterResult r = interpretNode(node, ast->call.args[i], env, error);
    if (r.flow != FLOW_NORMAL) return r;
    vals[nvals++] = r.value;
  }

  void *handle = NULL;
  size_t count = 1;

  if (nvals == 0) {
    /* new T() — 1 elemen, zeroed. */
    handle = gccalloc(1, (size_t)elemSize);
  } else if (nvals == 1) {
    /* new T(n) — n elemen, zeroed. */
    if (vals[0].type != VALUE_NUMBER || vals[0].as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(n)", "count must be a positive number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    count = (size_t)vals[0].as.number;
    handle = gccalloc(count, (size_t)elemSize);
  } else {
    /* new T(src, n) — realloc ke n elemen. */
    if (vals[0].type != VALUE_PTR || !vals[0].as.ptr) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(src, n)", "src must be a ptr");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (vals[1].type != VALUE_NUMBER || vals[1].as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(src, n)", "count must be a positive number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (gcfind(vals[0].as.ptr) < 0) {
      addRuntimeError(error, ERR_MEMORY, "new T(src, n)", "pointer not owned by GC");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    count = (size_t)vals[1].as.number;
    handle = gcarray(vals[0].as.ptr, count, (size_t)elemSize);
  }

  if (!handle) {
    addRuntimeError(error, ERR_INTERNAL, "new T()", "out of memory");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  return resultNormal(valuePtr(handle));
}

/* ==================== Contract ==================== */

/* Inti alokasi Contract: nama tipe (sudah dinormalisasi) + count elemen.
 * gccalloc(n, sizeof(T)) — zeroed; elemType tercatat di registry v3. */
static InterpreterResult contractAlloc(const char *type, size_t count, RuntimeValue *out,
                                       Error *error) {
  int elemSize = 0;
  if (!rupaMemorySizeOf(type, &elemSize) || elemSize <= 0) {
    char message[512];
    snprintf(message, sizeof(message), "unknown type '%s' in new Contract()", type);
    addRuntimeError(error, ERR_UNDEFINED_VAR, type, message);
    return resultFlow(FLOW_ERROR, valueNull());
  }
  void *handle = gccalloc(count, (size_t)elemSize);
  if (!handle) {
    addRuntimeError(error, ERR_INTERNAL, "new Contract()", "out of memory");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  gcregsettype(handle, type);
  *out = valuePtr(handle);
  return resultNormal(*out);
}

/* Contract di-resolve dari anotasi di SITE ASSIGNMENT (C1):
 * `p: T = new Contract(...)` — T dari anotasi, bukan dari argumen.
 * norm = true: anotasi 'T[]' direduksi ke elemen 'T' (C2: raw block);
 * tanpa anotasi = error. Return true + *out bila alokasi sukses.
 * *handled true berarti node memang `new Contract` (bukan jalur lain):
 * false + handled = error sudah ditulis (C1/arg/count); false + !handled
 * = bukan Contract, pemanggil lanjut jalur biasa. */
bool memoryContractAssign(Node *node, int valueId, const char *annType, bool norm, RuntimeEnv *env,
                          RuntimeValue *out, Error *error, bool *handled) {
  (void)norm;
  *handled = false;
  if (!node || valueId < 0 || valueId >= node->length) return false;
  AstNode *call = &node->ast[valueId];
  if (call->type != NODE_CALL || call->call.length < 1) return false;

  AstNode *callee = &node->ast[call->call.callee];
  const char *fn = callee->type == NODE_IDENTIFIER   ? callee->identifier.name
                   : callee->type == NODE_LITERAL_ID ? callee->string.value
                                                     : NULL;
  if (!fn || strcmp(fn, "new") != 0) return false;

  char raw[256];
  if (!newTypeArgName(node, call->call.args[0], raw, sizeof(raw))) return false;
  /* Buffer terpisah: newTypeNormalize(in, out) snprintf dengan in==out
   * adalah UB (overlap) — hasil string kosong. */
  char type[256];
  newTypeNormalize(raw, type, sizeof(type));
  if (strcmp(type, "Contract") != 0) return false; /* bukan Contract — jalur new biasa */
  *handled = true;
  /* C1: wajib anotasi. */
  if (!annType || !*annType) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract()",
                    "requires a type annotation (e.g. p: number = new Contract())");
    return false;
  }

  /* Tipe alokasi dari ANOTASI (bukan argumen 'Contract'). */
  snprintf(type, sizeof(type), "%s", annType);

  /* C2: anotasi 'T[]' → elemen 'T' (raw block, akses list[i]). */
  if (norm) {
    size_t len = strlen(type);
    if (len >= 2 && !strcmp(type + len - 2, "[]")) type[len - 2] = '\0';
  }

  /* args: [Contract], [Contract, count], atau [Contract, count,
   * elemsize]. Bentuk 3-arg = calloc custom (ukuran elemen eksplisit
   * — blok mentah, tanpa registrasi tipe elemen). */
  size_t count = 1;
  if (call->call.length > 3) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract(n)",
                    "expects at most (count, elemsize)");
    return false;
  }
  if (call->call.length == 3) {
    InterpreterResult r1 = interpretNode(node, call->call.args[1], env, error);
    if (r1.flow != FLOW_NORMAL) return false;
    InterpreterResult r2 = interpretNode(node, call->call.args[2], env, error);
    if (r2.flow != FLOW_NORMAL) return false;
    if (r1.value.type != VALUE_NUMBER || r1.value.as.number <= 0 ||
        r2.value.type != VALUE_NUMBER || r2.value.as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract(count, elemsize)",
                      "count and elemsize must be positive numbers");
      return false;
    }
    void *handle =
        gccalloc((size_t)r1.value.as.number, (size_t)r2.value.as.number);
    if (!handle) {
      addRuntimeError(error, ERR_INTERNAL, "new Contract(count, elemsize)", "out of memory");
      return false;
    }
    if (out) *out = valuePtr(handle);
    return true;
  }
  if (call->call.length == 2) {
    InterpreterResult r = interpretNode(node, call->call.args[1], env, error);
    if (r.flow != FLOW_NORMAL) return false;
    if (r.value.type != VALUE_NUMBER || r.value.as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract(n)",
                      "count must be a positive number");
      return false;
    }
    count = (size_t)r.value.as.number;
  }

  RuntimeValue v = valueNull();
  InterpreterResult r = contractAlloc(type, count, &v, error);
  if (r.flow != FLOW_NORMAL) return false;
  if (out) *out = v;
  return true;
}

/* ==================== register ==================== */

/* Nama tipe bila node adalah panggilan `new T(...)` — dipakai
 * NODE_ASSIGN/IR untuk kontrak type permanen: binding `x = new Number()`
 * mencatat declared type `number` sehingga `x = "str"` ditolak. */
bool memoryNewTypeName(Node *node, int valueId, char *buffer, size_t capacity) {
  if (!node || valueId < 0 || valueId >= node->length) return false;
  /* buffer boleh NULL — mode probe (hanya deteksi, tanpa tulis nama). */
  if (buffer && capacity == 0) return false;
  AstNode *call = &node->ast[valueId];
  if (call->type != NODE_CALL || call->call.length < 1) return false;
  AstNode *callee = &node->ast[call->call.callee];
  const char *fn = callee->type == NODE_IDENTIFIER   ? callee->identifier.name
                   : callee->type == NODE_LITERAL_ID ? callee->string.value
                                                     : NULL;
  if (!fn || strcmp(fn, "new") != 0) return false;
  char raw[256];
  if (!newTypeArgName(node, call->call.args[0], raw, sizeof(raw))) return false;
  if (buffer) newTypeNormalize(raw, buffer, capacity);
  /* Recontract bukan tipe: realloc tidak boleh menjadi kontrak binding
   * (`x = new Recontract(...)` tidak mencatat declared type). */
  if (buffer && strcmp(buffer, "Recontract") == 0) return false;
  return true;
}
