#include <rupa.h>

/* rupamemory.c — sistem memori rupa (global, tanpa import).
 *
 * sizeof(name)  — ukuran representasi tipe (scalar/struct/array).
 *                 Argumennya nama tipe, bukan value, jadi di-intercept
 *                 di interpretCall (pola push/pop) dan diteruskan ke
 *                 rupaMemorySizeOf.
 * pin/elpin/repin/repins — keluarga alokasi GC-tracked (lihat
 *                 design/rupa_memory_batch_1.txt):
 *                   pin    -> gcmall
 *                   elpin  -> gccalloc
 *                   repin  -> gcrealloc
 *                   repins -> gcarray (semantik reallocarray)
 *                 Handle VALUE_PTR opaque: disimpan, dibandingkan
 *                 (== null), dan dilempar kembali ke repin/repins.
 */

/* ==================== sizeof ==================== */

/* Ukuran representasi tipe rupa. Menerima nama scalar ("number"),
 * struct terdaftar, dan bentuk array "T[]" (array = {items, length}).
 * Return false bila tipe tidak dikenal. */
bool rupaMemorySizeOf(const char *type, int *outSize) {
  if (!type || !outSize) return false;

  /* Array: representasi = struct RuntimeArray (items + length). */
  size_t len = strlen(type);
  if (len >= 2 && strcmp(type + len - 2, "[]") == 0) {
    char elem[256];
    if (len - 2 >= sizeof(elem)) return false;
    memcpy(elem, type, len - 2);
    elem[len - 2] = '\0';
    int elemSize = 0;
    if (!rupaMemorySizeOf(elem, &elemSize)) return false;
    *outSize = (int)sizeof(struct RuntimeArray);
    return true;
  }

  if (!strcmp(type, "number")) {
    /* number 64-bit: VALUE_NUMBER = long long. */
    *outSize = (int)sizeof(long long);
    return true;
  }
  if (!strcmp(type, "decimal")) {
    *outSize = (int)sizeof(double);
    return true;
  }
  if (!strcmp(type, "boolean")) {
    *outSize = 1;
    return true;
  }
  if (!strcmp(type, "string")) {
    *outSize = (int)sizeof(char *);
    return true;
  }
  if (!strcmp(type, "ptr")) {
    *outSize = (int)sizeof(void *);
    return true;
  }

  /* Struct terdaftar: total ukuran field scalar-nya. */
  int structSize = 0;
  if (analyzerStructSizeOf(type, &structSize)) {
    *outSize = structSize;
    return true;
  }
  return false;
}

/* ==================== pin family ==================== */

static InterpreterResult builtinPin(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_NUMBER) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "pin(size) expects a number",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  void *handle = gcmall((size_t)argv[0].as.number);
  return resultNormal(valuePtr(handle));
}

static InterpreterResult builtinElpin(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || argv[0].type != VALUE_NUMBER || argv[1].type != VALUE_NUMBER) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "elpin(count, size) expects numbers",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  void *handle = gccalloc((size_t)argv[0].as.number, (size_t)argv[1].as.number);
  return resultNormal(valuePtr(handle));
}

static InterpreterResult builtinRepin(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || (argv[0].type != VALUE_PTR && argv[0].type != VALUE_NULL) ||
      argv[1].type != VALUE_NUMBER) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "repin(ptr, size) expects (ptr, number)",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (argv[0].type == VALUE_PTR && argv[0].as.ptr && gcfind(argv[0].as.ptr) < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "MemoryError",
                                  .message = "repin: pointer not owned by GC",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_INTERNAL});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  void *handle = gcrealloc(argv[0].as.ptr, (size_t)argv[1].as.number);
  return resultNormal(valuePtr(handle));
}

static InterpreterResult builtinRepins(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                       Error *error) {
  (void)env;
  if (argc < 3 || (argv[0].type != VALUE_PTR && argv[0].type != VALUE_NULL) ||
      argv[1].type != VALUE_NUMBER || argv[2].type != VALUE_NUMBER) {
    if (error)
      addError(error,
               (ErrorInfo){.code = "TypeError",
                           .message = "repins(ptr, count, size) expects (ptr, number, number)",
                           .line = 0,
                           .row = 0,
                           .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (argv[0].type == VALUE_PTR && argv[0].as.ptr && gcfind(argv[0].as.ptr) < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "MemoryError",
                                  .message = "repins: pointer not owned by GC",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_INTERNAL});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  void *handle = gcarray(argv[0].as.ptr, (size_t)argv[1].as.number, (size_t)argv[2].as.number);
  return resultNormal(valuePtr(handle));
}

/* unpin(ptr) — free eksplisit, setara free/gcfree. Statement-style:
 * unpin(p), tanpa assignment. Registry menghapus alamat sehingga
 * double-free dan unpin pada ptr yang sudah dibebaskan tertangkap
 * ("not owned by GC"). Alokasi bertahan tetap dirilis gcclean. */
static InterpreterResult builtinUnpin(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_PTR || !argv[0].as.ptr) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "unpin(ptr) expects a ptr",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (gcfind(argv[0].as.ptr) < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "MemoryError",
                                  .message = "unpin: pointer not owned by GC",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_MEMORY});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  gcfree(argv[0].as.ptr);
  return resultNormal(valueNull());
}

/* ==================== view type check ==================== */

/* Nama tipe dari argumen sizeof: identifier/literal, bentuk ekspresi
 * `T[]` (NODE_SUBSCRIPT kosong di konteks ekspresi), atau NODE_ARRAY_TYPE. */
static bool typeArgName(Node *node, int id, char *buffer, size_t capacity) {
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

/* Pin family: view type check (design/rupa_memory_batch_1.txt).
 * Handle dari pin/elpin dicek terhadap tipe anotasi lewat provenance
 * sizeof: p: number = pin(sizeof(number)) OK; pin(sizeof(string))
 * ditolak (dicek per TYPE, bukan per byte); annotation `T[]` dicocokkan
 * ke elemen-nya. Handle dari repin/repins inherit provenance lama.
 * pin tanpa sizeof = generic: hanya type "ptr" yang menerima.
 * Return true bila value bukan pin/elpin (bukan urusan check ini). */
bool memoryPinViewCheck(Node *node, int valueId, const char *type, Error *error) {
  if (!node || valueId < 0 || valueId >= node->length) return true;
  AstNode *call = &node->ast[valueId];
  if (call->type != NODE_CALL || call->call.length < 1) return true;

  AstNode *callee = &node->ast[call->call.callee];
  const char *fn = callee->type == NODE_IDENTIFIER   ? callee->identifier.name
                   : callee->type == NODE_LITERAL_ID ? callee->string.value
                                                     : NULL;
  /* repin/repins: handle inherit provenance dari pointer lama. */
  if (!fn || (strcmp(fn, "pin") && strcmp(fn, "elpin"))) return true;

  /* Provenance: sizeof(T) di salah satu argumen pin/elpin
   * (pin: args[0]; elpin: args[1]). */
  char provenance[256];
  bool has = false;
  for (int i = 0; i < call->call.length && !has; i++) {
    AstNode *arg = &node->ast[call->call.args[i]];
    if (arg->type != NODE_CALL || arg->call.length != 1) continue;
    AstNode *inner = &node->ast[arg->call.callee];
    const char *innerFn = inner->type == NODE_IDENTIFIER   ? inner->identifier.name
                          : inner->type == NODE_LITERAL_ID ? inner->string.value
                                                           : NULL;
    if (innerFn && !strcmp(innerFn, "sizeof"))
      has = typeArgName(node, arg->call.args[0], provenance, sizeof(provenance));
  }

  if (!has) {
    if (type && !strcmp(type, "ptr")) return true;
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, type ? type : "ptr",
                      "pin without sizeof produces a generic ptr");
    return false;
  }

  /* Annotation "ptr" = handle generik: menerima pin apa pun. */
  if (type && !strcmp(type, "ptr")) return true;

  /* Annotation bentuk elemen `T[]`: bandingkan elemen-nya. */
  const char *expected = type;
  char element[256];
  if (type) {
    size_t len = strlen(type);
    if (len >= 2 && !strcmp(type + len - 2, "[]") && len - 2 < sizeof(element)) {
      memcpy(element, type, len - 2);
      element[len - 2] = '\0';
      expected = element;
    }
  }

  if (!expected || !strcmp(expected, provenance)) return true;
  if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, type, provenance);
  return false;
}

/* ==================== operasi blok (batch 2) ==================== */

/* Validasi posisi tulis: wajib VALUE_PTR milik registry GC.
 * Posisi baca: ptr GC atau string biasa (read-only). Return NULL bila
 * valid (mengisi *out), selain itu pesan error sudah ditulis. */
static const void *memArgRead(RuntimeValue v, bool write, const char *fn, Error *error,
                              const void **out) {
  (void)fn;
  if (v.type == VALUE_PTR) {
    if (v.as.ptr && gcfind(v.as.ptr) < 0) {
      if (error)
        addError(error, (ErrorInfo){.code = "MemoryError",
                                    .message = "pointer not owned by GC",
                                    .line = 0,
                                    .row = 0,
                                    .type = ERR_MEMORY});
      return NULL;
    }
    *out = v.as.ptr;
    return *out;
  }
  if (!write && v.type == VALUE_STRING) {
    *out = v.as.string;
    return *out;
  }
  if (error)
    addError(error, (ErrorInfo){.code = "TypeError",
                                .message = write ? "expects a ptr (GC-owned)"
                                                 : "expects a ptr (GC-owned) or string",
                                .line = 0,
                                .row = 0,
                                .type = ERR_TYPE_MISMATCH});
  return NULL;
}

static int memArgSize(RuntimeValue v, const char *fn, Error *error) {
  (void)fn;
  if (v.type != VALUE_NUMBER || v.as.number < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "expects a byte count (number)",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return -1;
  }
  return v.as.number;
}

/* copypin(dest, src, n) — memcpy; return dest. */
static InterpreterResult builtinCopypin(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                        Error *error) {
  (void)env;
  const void *src = NULL;
  const void *dest = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "copypin", error, &dest) ||
      !memArgRead(argv[1], false, "copypin", error, &src) ||
      memArgSize(argv[2], "copypin", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  gccpy((void *)dest, src, n);
  return resultNormal(valuePtr((void *)dest));
}

/* movepin(dest, src, n) — memmove (aman overlap); return dest. */
static InterpreterResult builtinMovepin(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                        Error *error) {
  (void)env;
  const void *src = NULL;
  const void *dest = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "movepin", error, &dest) ||
      !memArgRead(argv[1], false, "movepin", error, &src) ||
      memArgSize(argv[2], "movepin", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  gcmove((void *)dest, src, n);
  return resultNormal(valuePtr((void *)dest));
}

/* setpin(ptr, value, n) — memset; return ptr. */
static InterpreterResult builtinSetpin(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                       Error *error) {
  (void)env;
  const void *ptr = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "setpin", error, &ptr) ||
      argv[1].type != VALUE_NUMBER || memArgSize(argv[2], "setpin", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  gcset((void *)ptr, (int)argv[1].as.number, n);
  return resultNormal(valuePtr((void *)ptr));
}

/* pincmp(a, b, n) — memcmp; < 0 / 0 / > 0. Baca: ptr GC atau string. */
static InterpreterResult builtinPincmp(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                       Error *error) {
  (void)env;
  const void *a = NULL;
  const void *b = NULL;
  if (argc < 3 || !memArgRead(argv[0], false, "pincmp", error, &a) ||
      !memArgRead(argv[1], false, "pincmp", error, &b) || memArgSize(argv[2], "pincmp", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  return resultNormal(valueNumber(gccmp(a, b, n)));
}

/* dupin(str) — strdup ke memori terdaftar GC; return ptr. */
static InterpreterResult builtinDupin(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "dupin(str) expects a string",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  char *copy = gcdup(argv[0].as.string ? argv[0].as.string : "");
  return resultNormal(valuePtr(copy));
}

/* maxdupin(str, n) — strndup maksimal n karakter, selalu NUL-terminated
 * (n + 1 byte); return ptr. */
static InterpreterResult builtinMaxdupin(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                         Error *error) {
  (void)env;
  if (argc < 2 || argv[0].type != VALUE_STRING || argv[1].type != VALUE_NUMBER ||
      argv[1].as.number < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "maxdupin(str, n) expects (string, number)",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  char *copy = gcndup(argv[0].as.string ? argv[0].as.string : "", (size_t)argv[1].as.number);
  return resultNormal(valuePtr(copy));
}

/* Register rupa memory builtins (pin family) — global, tanpa import. */
void rupaMemoryInit(RuntimeEnv *env) {
  if (!env) return;
  semSet(env, "pin", valueNativeFunction("pin", builtinPin, 1));
  semSet(env, "elpin", valueNativeFunction("elpin", builtinElpin, 2));
  semSet(env, "repin", valueNativeFunction("repin", builtinRepin, 2));
  semSet(env, "repins", valueNativeFunction("repins", builtinRepins, 3));
  semSet(env, "unpin", valueNativeFunction("unpin", builtinUnpin, 1));
  // operasi blok (batch 2)
  semSet(env, "copypin", valueNativeFunction("copypin", builtinCopypin, 3));
  semSet(env, "movepin", valueNativeFunction("movepin", builtinMovepin, 3));
  semSet(env, "setpin", valueNativeFunction("setpin", builtinSetpin, 3));
  semSet(env, "pincmp", valueNativeFunction("pincmp", builtinPincmp, 3));
  // alokasi string (batch 2)
  semSet(env, "dupin", valueNativeFunction("dupin", builtinDupin, 1));
  semSet(env, "maxdupin", valueNativeFunction("maxdupin", builtinMaxdupin, 2));
}
