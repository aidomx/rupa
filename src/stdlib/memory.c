#include <rupa.h>

/* memory.c — sistem memori type-driven: new/del (design/new_memory.txt).
 *
 * Layer di atas gc.c yang MEMANFAATKAN pengetahuan tipe:
 *   new Number()        — 1 elemen, ukuran otomatis sizeof(number) (zeroed)
 *   new Number(n)       — n elemen, zeroed
 *   new Number(src, n)  — realloc ke n elemen (handle lama dangling)
 *   del(x, y, ...) / del([x, y]) — free variadic / dari array
 *
 * Kapitalisasi PENTING: bentuknya `new Number()`, bukan `new number()`
 * — `new number()` bentrok dengan `x: number` (penanda tipe). Argument
 * new SELALU nama tipe (Number/String/Struct), tidak pernah dievaluasi.
 *
 * `n` = jumlah ELEMEN, bukan byte — byte tidak pernah muncul di API.
 * `del` tanpa `&` — handle opaque, guard gcfind (double-free tertangkap).
 * Zeroed (calloc-style): tidak ada state uninitialized.
 */

/* ==================== new ==================== */

/* Nama tipe dari node argumen `new`: identifier/literal, bentuk `T[]`
 * (NODE_SUBSCRIPT kosong di konteks ekspresi), atau NODE_ARRAY_TYPE. */
static bool newTypeArgName(Node *node, int id, char *buffer, size_t capacity) {
  if (!node || id < 0 || id >= node->length || !buffer || capacity == 0)
    return false;
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
      {"Number", "number"}, {"String", "string"}, {"Boolean", "boolean"},
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
  if (ast->call.length < 1 || ast->call.args[0] < 0 ||
      ast->call.args[0] >= node->length)
    return resultNormal(valueNull());

  /* Arg[0] = nama tipe, tidak dievaluasi. */
  char typeRaw[256];
  if (!newTypeArgName(node, ast->call.args[0], typeRaw, sizeof(typeRaw)))
    return resultNormal(valueNull());
  char type[256];
  newTypeNormalize(typeRaw, type, sizeof(type));

  int elemSize = 0;
  if (!rupaMemorySizeOf(type, &elemSize) || elemSize <= 0) {
    if (!strcmp(type, "Contract")) {
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
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T()",
                      "expects at most 2 numbers (src, n)");
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
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(n)",
                      "count must be a positive number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    count = (size_t)vals[0].as.number;
    handle = gccalloc(count, (size_t)elemSize);
  } else {
    /* new T(src, n) — realloc ke n elemen. */
    if (vals[0].type != VALUE_PTR || !vals[0].as.ptr) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(src, n)",
                      "src must be a ptr");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (vals[1].type != VALUE_NUMBER || vals[1].as.number <= 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "new T(src, n)",
                      "count must be a positive number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (gcfind(vals[0].as.ptr) < 0) {
      addRuntimeError(error, ERR_MEMORY, "new T(src, n)",
                      "pointer not owned by GC");
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

/* ==================== del ==================== */

/* del(handle) — satu handle; return null. Guard: ptr milik GC;
 * view handle ditolak (kepemilikan ada di blok owner). */
static InterpreterResult delHandle(RuntimeValue v, Error *error) {
  if (v.type != VALUE_PTR || !v.as.ptr) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "del(x)",
                    "expects a ptr (GC-owned)");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (gcregisview(v.as.ptr)) {
    addRuntimeError(error, ERR_MEMORY, "del(x)",
                    "handle is a view into a struct block — delete the owner instead");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (gcfind(v.as.ptr) < 0) {
    addRuntimeError(error, ERR_MEMORY, "del(x)", "pointer not owned by GC");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  gcfree(v.as.ptr);
  return resultNormal(valueNull());
}

/* del(x, y, ...) / del([x, y]) — free variadic / dari array. */
InterpreterResult memoryDelCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error, bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  const char *name = NULL;
  if (calleeAst->type == NODE_IDENTIFIER)
    name = calleeAst->identifier.name;
  else if (calleeAst->type == NODE_LITERAL_ID)
    name = calleeAst->string.value;
  if (!name || strcmp(name, "del") != 0) return resultNormal(valueNull());

  *handled = true;
  if (ast->call.length < 1)
    return resultNormal(valueNull());

  for (int i = 0; i < ast->call.length; i++) {
    InterpreterResult r = interpretNode(node, ast->call.args[i], env, error);
    if (r.flow != FLOW_NORMAL) return r;

    if (r.value.type == VALUE_ARRAY) {
      /* del([x, y]) — tiap elemen array adalah handle. */
      for (int j = 0; j < r.value.as.array.length; j++) {
        InterpreterResult d = delHandle(r.value.as.array.items[j], error);
        if (d.flow != FLOW_NORMAL) return d;
      }
      continue;
    }
    InterpreterResult d = delHandle(r.value, error);
    if (d.flow != FLOW_NORMAL) return d;
  }
  return resultNormal(valueNull());
}

/* ==================== indexing VALUE_PTR ==================== */

/* Inti value-based — dipakai interpreter (setelah evaluasi target/index)
 * dan IR machine (nilai langsung dari register). Return false + error
 * (dan *fatal = true) bila akses tidak valid. */
static bool memoryPtrAccess(void *ptr, RuntimeValue idx, bool isWrite, RuntimeValue val,
                            RuntimeValue *out, Error *error, bool *fatal) {
  *fatal = false;
  if (!ptr || gcfind(ptr) < 0) {
    if (error) {
      addRuntimeError(error, ERR_MEMORY, isWrite ? "x[i] = v" : "x[i]",
                      "pointer not owned by GC");
    }
    *fatal = true;
    return false;
  }
  if (idx.type != VALUE_NUMBER) {
    if (error) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, isWrite ? "x[i] = v" : "x[i]",
                      "index must be a number");
    }
    *fatal = true;
    return false;
  }
  long long i = idx.as.number;
  size_t bytes = gcsize(ptr);
  size_t count = gcelem(ptr);
  size_t elems = count > 0 ? count : (bytes > 0 ? 1 : 0);
  size_t elemBytes = elems > 0 ? bytes / elems : 0;

  if (i < 0 || (size_t)i >= elems) {
    char message[128];
    snprintf(message, sizeof(message),
             "index %lld is out of bounds for block of %zu element(s)", i,
             elems);
    if (error) {
      addError(error, (ErrorInfo){.code = "RangeError",
                                  .message = gcdup(message),
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_INDEX_OUT_OF_BOUNDS});
    }
    *fatal = true;
    return false;
  }

  /* Blok struct (C-B3): baca elemen ke-i → view handle; field diakses
   * per elemen lewat member access (arr[i].x). Tulis seluruh elemen
   * tetap dijalur lama (ditolak eksplisit untuk layout-mixed). */
  if (!isWrite) {
    const char *stype = gcregtype(ptr);
    if (stype && analyzerFindStruct(stype) && elemBytes > 0) {
      void *view = gcregview(ptr, (size_t)i * elemBytes);
      if (!view || !gcregsettype(view, stype)) {
        if (error)
          addRuntimeError(error, ERR_MEMORY, "x[i]",
                          "cannot create element view (out of memory)");
        *fatal = true;
        return false;
      }
      if (out) *out = valuePtr(view);
      return true;
    }
  }

  char *dest = (char *)ptr + (size_t)i * elemBytes;
  if (isWrite) {
    /* number 64-bit: elemen number = long long (8 byte). */
    if (elemBytes == sizeof(long long)) {
      if (val.type != VALUE_NUMBER) {
        if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i] = v", "expects a number");
        *fatal = true;
        return false;
      }
      long long v = val.as.number;
      memcpy(dest, &v, sizeof(v));
      if (out) *out = val;
      return true;
    }
    if (elemBytes == sizeof(int)) {
      if (val.type != VALUE_NUMBER) {
        if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i] = v", "expects a number");
        *fatal = true;
        return false;
      }
      int v = (int)val.as.number;
      memcpy(dest, &v, sizeof(v));
      if (out) *out = val;
      return true;
    }
    if (elemBytes == sizeof(double)) {
      if (val.type != VALUE_NUMBER && val.type != VALUE_DECIMAL) {
        if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i] = v", "expects a number");
        *fatal = true;
        return false;
      }
      double d = val.type == VALUE_DECIMAL ? val.as.decimal : (double)val.as.number;
      memcpy(dest, &d, sizeof(d));
      if (out) *out = val;
      return true;
    }
    if (elemBytes == 1) {
      if (val.type != VALUE_NUMBER) {
        if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i] = v", "expects a number");
        *fatal = true;
        return false;
      }
      *dest = (char)val.as.number;
      if (out) *out = val;
      return true;
    }
    if (error) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i] = v",
                      "element type not writable via indexing");
    }
    *fatal = true;
    return false;
  }

  /* Baca elemen sesuai elemBytes (provenance elemen-nya).
   * number 64-bit: elemen number = long long (8 byte). */
  if (elemBytes == sizeof(long long)) {
    long long v;
    memcpy(&v, dest, sizeof(v));
    if (out) *out = valueNumber(v);
    return true;
  }
  if (elemBytes == sizeof(int)) {
    int v;
    memcpy(&v, dest, sizeof(v));
    if (out) *out = valueNumber(v);
    return true;
  }
  if (elemBytes == sizeof(double)) {
    double d;
    memcpy(&d, dest, sizeof(d));
    if (out) *out = valueDecimal(d);
    return true;
  }
  if (elemBytes == 1) {
    if (out) *out = valueNumber((int)*dest);
    return true;
  }
  if (error) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i]",
                    "element type not readable via indexing");
  }
  *fatal = true;
  return false;
}

/* x[i] — baca elemen handle: scalar (i == 0) atau blok T (0 <= i < n). */
InterpreterResult memoryIndexGet(Node *node, int targetId, int indexId,
                                 RuntimeEnv *env, Error *error, bool *handled) {
  *handled = false;
  if (!node || targetId < 0 || targetId >= node->length)
    return resultNormal(valueNull());

  InterpreterResult target = interpretNode(node, targetId, env, error);
  if (target.flow != FLOW_NORMAL) return target;
  if (target.value.type != VALUE_PTR) return resultNormal(valueNull());

  *handled = true;
  InterpreterResult idx = interpretNode(node, indexId, env, error);
  if (idx.flow != FLOW_NORMAL) return idx;

  RuntimeValue out = valueNull();
  bool fatal = false;
  if (!memoryPtrAccess(target.value.as.ptr, idx.value, false, valueNull(), &out,
                       error, &fatal))
    return resultFlow(FLOW_ERROR, valueNull());
  return resultNormal(out);
}

/* x[i] = v — tulis elemen handle (scalar index 0 atau blok). */
InterpreterResult memoryIndexSet(Node *node, int targetId, int indexId,
                                 RuntimeValue val, RuntimeEnv *env,
                                 Error *error, bool *handled) {
  *handled = false;
  if (!node || targetId < 0 || targetId >= node->length)
    return resultNormal(valueNull());

  InterpreterResult target = interpretNode(node, targetId, env, error);
  if (target.flow != FLOW_NORMAL) return target;
  if (target.value.type != VALUE_PTR) return resultNormal(valueNull());

  *handled = true;
  InterpreterResult idx = interpretNode(node, indexId, env, error);
  if (idx.flow != FLOW_NORMAL) return idx;

  RuntimeValue out = valueNull();
  bool fatal = false;
  if (!memoryPtrAccess(target.value.as.ptr, idx.value, true, val, &out, error, &fatal))
    return resultFlow(FLOW_ERROR, valueNull());
  return resultNormal(out);
}

/* ==================== member access struct (C3) ==================== */

/* Baca field struct pada handle ptr: offset dari layout analyzer
 * (C-style: field diselaskan pada alignment-nya), decode sesuai tipe
 * field. string/array = slot/snapshot; struct bertingkat = view handle. */
static bool memberFieldRead(void *ptr, const char *structType, const char *field,
                            RuntimeValue *out, Error *error) {
  int offset = 0;
  char ftype[256];
  if (!analyzerFieldOffset(structType, field, &offset, ftype, sizeof(ftype))) {
    char message[512];
    snprintf(message, sizeof(message), "unknown field '%s' on struct '%s'", field,
             structType);
    addRuntimeError(error, ERR_TYPE_MISMATCH, structType, message);
    return false;
  }
  const char *src = (const char *)ptr + offset;

  if (!strcmp(ftype, "number")) {
    /* number 64-bit: layout struct menyimpan long long (8 byte). */
    long long v;
    memcpy(&v, src, sizeof(v));
    *out = valueNumber(v);
    return true;
  }
  if (!strcmp(ftype, "decimal")) {
    double d;
    memcpy(&d, src, sizeof(d));
    *out = valueDecimal(d);
    return true;
  }
  if (!strcmp(ftype, "boolean")) {
    *out = valueBoolean(*src != 0);
    return true;
  }
  /* String slot FIRST-CLASS (design/str_memory.txt): buffer scalar
   * menyimpan char* di slot — baca deref. */
  if (!strcmp(ftype, "string")) {
    char *saved;
    memcpy(&saved, src, sizeof(saved));
    *out = valueString(saved ? saved : "");
    return true;
  }

  /* Struct bertingkat (C-B2): field struct → view handle ke posisi
   * field — member access berikutnya (field/elemen) bekerja di atas
   * view; kepemilikan tetap di blok owner. View bertipe struct tujuan
   * sehingga bisa di-assign ke variable beranotasi. */
  if (analyzerFindStruct(ftype)) {
    void *view = gcregview(ptr, (size_t)offset);
    if (!view || !gcregsettype(view, ftype)) {
      addRuntimeError(error, ERR_MEMORY, structType,
                      "cannot create field view (out of memory)");
      return false;
    }
    *out = valuePtr(view);
    return true;
  }

  /* Kompleks lain (ptr/array): buffer scalar hanya menyimpan byte
   * mentah; konversi jadi object snapshot read-only. */
  size_t bytes = gcsize(ptr);
  size_t count = gcelem(ptr);
  size_t elems = count > 0 ? count : (bytes > 0 ? 1 : 0);
  size_t elemBytes = elems > 0 ? bytes / elems : 0;
  size_t avail = bytes > (size_t)offset ? bytes - (size_t)offset : 0;
  size_t span = elemBytes && (size_t)offset + elemBytes <= bytes ? elemBytes : avail;
  RuntimeValue obj = valueObject(NULL);
  valueObjectSet(&obj, "field", valueString(gcstrdup(field)));
  valueObjectSet(&obj, "type", valueString(gcstrdup(ftype)));
  valueObjectSet(&obj, "bytes", valueNumber((int)span));
  valueObjectSet(&obj, "note",
                 valueString(gcstrdup("raw field snapshot (buffer-scalar)")));
  *out = obj;
  return true;
}

/* Tulis field struct pada handle ptr: encode sesuai tipe field.
 * number/decimal/boolean ditulis native; tipe kompleks ditolak. */
static bool memberFieldWrite(void *ptr, const char *structType, const char *field,
                             RuntimeValue val, Error *error) {
  int offset = 0;
  char ftype[256];
  if (!analyzerFieldOffset(structType, field, &offset, ftype, sizeof(ftype))) {
    char message[512];
    snprintf(message, sizeof(message), "unknown field '%s' on struct '%s'", field,
             structType);
    addRuntimeError(error, ERR_TYPE_MISMATCH, structType, message);
    return false;
  }
  char *dest = (char *)ptr + offset;

  if (!strcmp(ftype, "number")) {
    if (val.type != VALUE_NUMBER) goto typefail;
    long long v = val.as.number;
    memcpy(dest, &v, sizeof(v));
    return true;
  }
  if (!strcmp(ftype, "decimal")) {
    if (val.type != VALUE_NUMBER && val.type != VALUE_DECIMAL) goto typefail;
    double d = val.type == VALUE_DECIMAL ? val.as.decimal : (double)val.as.number;
    memcpy(dest, &d, sizeof(d));
    return true;
  }
  if (!strcmp(ftype, "boolean")) {
    if (val.type != VALUE_BOOLEAN) goto typefail;
    *dest = (char)(val.as.boolean ? 1 : 0);
    return true;
  }
  /* String slot FIRST-CLASS (design/str_memory.txt): tulis char* ke
   * slot. RHS string biasa -> gcstrdup (GC-tracked); RHS ptr milik
   * GC (dupl) -> pointer disimpan langsung. */
  if (!strcmp(ftype, "string")) {
    char *slotp = NULL;
    memcpy(&slotp, dest, sizeof(slotp));
    if (val.type == VALUE_STRING) {
      char *copy = gcstrdup(val.as.string ? val.as.string : "");
      if (!copy) return false;
      memcpy(dest, &copy, sizeof(copy));
      return true;
    }
    if (val.type == VALUE_PTR && val.as.ptr && gcfind(val.as.ptr) >= 0) {
      memcpy(dest, &val.as.ptr, sizeof(val.as.ptr));
      return true;
    }
    (void)slotp;
    goto typefail;
  }

  /* Struct bertingkat (C-B2): RHS blok/view struct lain → copy byte
   * sebanyak sizeof(ftype) — semantik struct assignment C. */
  if (analyzerFindStruct(ftype)) {
    if (val.type == VALUE_PTR && val.as.ptr &&
        (gcfind(val.as.ptr) >= 0 || gcregisview(val.as.ptr))) {
      int srcSize = 0;
      if (analyzerStructSizeOf(ftype, &srcSize) && srcSize > 0) {
        memcpy(dest, val.as.ptr, (size_t)srcSize);
        return true;
      }
    }
    char message[512];
    snprintf(message, sizeof(message),
             "field '%s' expects a struct handle/view of '%s'", field, ftype);
    addRuntimeError(error, ERR_TYPE_MISMATCH, ftype, message);
    return false;
  }

typefail:
  {
    char message[512];
    snprintf(message, sizeof(message),
             "field '%s' of type '%s' is not writable via handle (complex field)",
             field, ftype);
    addRuntimeError(error, ERR_TYPE_MISMATCH, ftype, message);
    return false;
  }
}

/* Helper shared interpreter+IR: resolve structType handle (registry v3
 * + view), dispatch get/set. View handle (blok header {owner, offset}):
 * tipe dari view sendiri, akses byte di owner + viewOffset. Return
 * false (fatal=false) bila handle bukan struct — jalur lama lanjut. */
static bool structMemberAccess(void *ptr, const char *field, bool isWrite, RuntimeValue val,
                               RuntimeValue *out, Error *error, bool *fatal) {
  *fatal = false;
  void *target = ptr;
  const char *stype = gcregtype(ptr);
  if (gcelem(ptr) == GC_VIEW_MAGIC) {
    /* View: {owner, offset} di header; data struct ada di owner+offset. */
    void *owner = NULL;
    size_t voff = 0;
    memcpy(&owner, ptr, sizeof(owner));
    memcpy(&voff, (char *)ptr + sizeof(owner), sizeof(voff));
    if (!owner) {
      if (error)
        addRuntimeError(error, ERR_MEMORY, isWrite ? "obj.f = v" : "obj.f",
                        "view handle is dangling — owner block was deleted");
      *fatal = true;
      return false;
    }
    target = (char *)owner + voff;
    if (!stype) stype = gcregtype(owner); /* fallback: tipe owner */
  }
  if (!stype || !analyzerFindStruct(stype))
    return false; /* bukan struct handle — jalur lama */
  if (isWrite) {
    if (!memberFieldWrite(target, stype, field, val, error)) {
      *fatal = true;
      return false;
    }
    if (out) *out = val;
    return true;
  }
  if (!memberFieldRead(target, stype, field, out, error)) {
    *fatal = true;
    return false;
  }
  return true;
}

/* Entry member access (dipanggil interpretMember / IR_MEMBER_GET).
 * Return false bila target bukan struct handle — jalur lama lanjut. */
bool memoryMemberGet(void *ptr, const char *field, RuntimeValue *out, Error *error,
                     bool *fatal) {
  return structMemberAccess(ptr, field, false, valueNull(), out, error, fatal);
}

bool memoryMemberSet(void *ptr, const char *field, RuntimeValue val, Error *error,
                     bool *fatal) {
  RuntimeValue out = valueNull();
  return structMemberAccess(ptr, field, true, val, &out, error, fatal);
}

/* ==================== dupl / compare (design/str_memory.txt) ==================== */

/* Write-through string-slot (keputusan design): value string/dupl-ptr
 * DITULIS ke slot handle Contract string — handle tetap valid;
 * ptr lain = rebind biasa (handle swap). Return false bila bukan
 * kasus write-through (pemanggil lanjut rebind). */
bool memoryStringSlotWrite(const char *name, void *ptr, RuntimeValue val, Error *error) {
  (void)error;
  const char *stype = gcregtype(ptr);
  if (!stype || strcmp(stype, "string") != 0) return false;
  size_t bytes = gcsize(ptr);
  if (bytes < sizeof(char *)) return false;

  char *copy = NULL;
  if (val.type == VALUE_STRING) {
    copy = gcstrdup(val.as.string ? val.as.string : "");
  } else if (val.type == VALUE_PTR && val.as.ptr && gcfind(val.as.ptr) >= 0 &&
             !gcregtype(val.as.ptr)) {
    /* RHS ptr TANPA tipe terdaftar (hasil dupl) — pointer dipindah ke
     * slot. Handle Contract (typed) → false: pemanggil melakukan rebind. */
    copy = (char *)val.as.ptr;
  }
  if (!copy) return false;
  memcpy(ptr, &copy, sizeof(copy));
  (void)name;
  return true;
}

/* Read-through string slot (keputusan design): handle Contract string
 * DIBACA sebagai VALUE_STRING dari slot — bukan handle mentah. Return
 * false bila bukan kasus read-through (pemanggil lanjut handle). */
bool memoryStringSlotRead(void *ptr, RuntimeValue *out, Error *error) {
  (void)error;
  const char *stype = gcregtype(ptr);
  if (!stype || strcmp(stype, "string") != 0) return false;
  size_t bytes = gcsize(ptr);
  if (bytes < sizeof(char *)) return false;
  char *saved = NULL;
  memcpy(&saved, ptr, sizeof(saved));
  *out = valueString(saved ? saved : "");
  return true;
}

/* dupl(str) — strdup GC-tracked; dupl(str, n) — strndup maksimal n
 * char, selalu NUL-terminated. Menggantikan dupin/maxdupin. */
static InterpreterResult builtinDupl(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                     Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "dupl(str)",
                    "expects a string");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  const char *s = argv[0].as.string ? argv[0].as.string : "";
  char *copy = NULL;
  if (argc >= 2) {
    if (argv[1].type != VALUE_NUMBER || argv[1].as.number < 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "dupl(str, n)",
                      "n must be a non-negative number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    copy = gcstrndup(s, (size_t)argv[1].as.number);
  } else {
    copy = gcstrdup(s);
  }
  return resultNormal(valuePtr(copy));
}

/* compare(a, b) — strcmp-style untuk string (handle slot & string
 * biasa); compare(a, b, n) — memcmp untuk blok mentah. Menggantikan
 * pincmp untuk kasus string. */
static InterpreterResult builtinCompare(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                        Error *error) {
  (void)env;
  if (argc < 2) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b)",
                    "expects two string/ptr arguments");
    return resultFlow(FLOW_ERROR, valueNull());
  }

  const char *as = NULL;
  const char *bs = NULL;
  if (argv[0].type == VALUE_STRING) as = argv[0].as.string;
  else if (argv[0].type == VALUE_PTR && argv[0].as.ptr && gcfind(argv[0].as.ptr) >= 0)
    as = (const char *)argv[0].as.ptr;
  if (argv[1].type == VALUE_STRING) bs = argv[1].as.string;
  else if (argv[1].type == VALUE_PTR && argv[1].as.ptr && gcfind(argv[1].as.ptr) >= 0)
    bs = (const char *)argv[1].as.ptr;
  if (!as || !bs) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b)",
                    "expects strings or GC-owned handles");
    return resultFlow(FLOW_ERROR, valueNull());
  }

  if (argc >= 3) {
    if (argv[2].type != VALUE_NUMBER || argv[2].as.number < 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b, n)",
                      "n must be a non-negative number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    return resultNormal(valueNumber(gccmp(as, bs, (size_t)argv[2].as.number)));
  }
  return resultNormal(valueNumber(strcmp(as, bs)));
}

/* Versi value-based untuk IR machine — nilai sudah di register. */
RuntimeValue memoryPtrGet(RuntimeValue ptr, RuntimeValue idx, Error *error, bool *fatal) {
  RuntimeValue out = valueNull();
  if (ptr.type != VALUE_PTR ||
      !memoryPtrAccess(ptr.as.ptr, idx, false, valueNull(), &out, error, fatal))
    return valueNull();
  return out;
}

bool memoryPtrSet(RuntimeValue ptr, RuntimeValue idx, RuntimeValue val, Error *error,
                  bool *fatal) {
  if (ptr.type != VALUE_PTR) return false;
  RuntimeValue out = valueNull();
  return memoryPtrAccess(ptr.as.ptr, idx, true, val, &out, error, fatal);
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
bool memoryContractAssign(Node *node, int valueId, const char *annType, bool norm,
                          RuntimeEnv *env, RuntimeValue *out, Error *error,
                          bool *handled) {
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
    if (len >= 2 && !strcmp(type + len - 2, "[]"))
      type[len - 2] = '\0';
  }

  /* args: [Contract] atau [Contract, count]. Realloc bukan wilayah
   * Contract — pakai new T(src, n). */
  size_t count = 1;
  if (call->call.length > 2) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "new Contract(n)",
                    "expects at most one count argument");
    return false;
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
  return true;
}

void memoryInit(RuntimeEnv *env) {
  if (!env) return;
  /* new/del tidak di-register sebagai native function biasa:
   * arg pertamanya nama tipe (tidak dievaluasi), jadi di-intercept
   * di interpretCall via memoryNewCall/memoryDelCall. */
  /* dupl/compare (design/str_memory.txt) — global, tanpa import. */
  semSet(env, "dupl", valueNativeFunction("dupl", builtinDupl, 1));
  semSet(env, "compare", valueNativeFunction("compare", builtinCompare, 2));
}
