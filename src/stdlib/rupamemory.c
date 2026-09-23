#include <rupa.h>

/* rupamemory.c — operasi memori rupa (global, tanpa import).
 *
 * SUNSET pin/elpin/repin/repins (design/new_memory.txt keputusan #4:
 * Replace + deprecated; design/str_memory.txt — "new Contract() sudah
 * sangat cocok"): satu-satunya API alokasi kini new/del/Contract
 * (src/stdlib/memory.c). File ini kini hanya menampung OPERASI BLOK
 * (batch 2): cset/cmove/ccpy (lapisan rendah di atas
 * handle Contract) + dup family (dupl; dupin/maxdupin masih berfungsi
 * tapi TIDAK di-expose di docs).
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
}/* ==================== view type check handle ==================== */

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

/* View type check handle VALUE_PTR — SEKARANG berbasis registry v3
 * (design/new_memory.txt C4: "view check jadi lookup; provenance sizeof
 * pin jadi legacy"). Tipe handle tercatat saat alokasi:
 *   - new T() / Contract: gcregsettype(handle, T) — lookup langsung
 *   - dupl/dupin: ptr TANPA tipe — sesuai design string-slot, hanya
 *     boleh masuk variable bertipe "ptr" atau "string" (handle slot
 *     read/write-through sebagai string).
 * Return true bila value bukan handle GC (bukan urusan check ini). */
bool memoryHandleTypeCheck(RuntimeValue value, const char *type, Error *error) {
  if (!type || !*type) return true;

  if (value.type != VALUE_PTR || !value.as.ptr) return true;

  const char *have = gcregtype(value.as.ptr);
  if (have) {
    /* Anotasi bentuk elemen `T[]`: cocokkan elemen-nya (raw block). */
    const char *expected = type;
    char element[256];
    size_t len = strlen(type);
    if (len >= 2 && !strcmp(type + len - 2, "[]") && len - 2 < sizeof(element)) {
      memcpy(element, type, len - 2);
      element[len - 2] = '\0';
      expected = element;
    }
    if (!strcmp(expected, have) || !strcmp(expected, "ptr")) return true;
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, type, have);
    return false;
  }

  /* Handle tanpa tipe (dupl/dupin): design str_memory.txt — slot string
   * transparan; masuk variable "string"/"ptr" OK, selain itu tolak. */
  if (!strcmp(type, "string") || !strcmp(type, "ptr")) return true;
  if (error)
    addRuntimeError(error, ERR_TYPE_MISMATCH, type,
                    "untyped string handle (dupl) — use new Contract() for typed buffers");
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

/* ccpy(dest, src, n) — memcpy pada handle; return dest.
 * (dulu copypin — rename ke identitas contract, momen registry v3.) */
static InterpreterResult builtinCcpy(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                     Error *error) {
  (void)env;
  const void *src = NULL;
  const void *dest = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "ccpy", error, &dest) ||
      !memArgRead(argv[1], false, "ccpy", error, &src) ||
      memArgSize(argv[2], "ccpy", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  gccpy((void *)dest, src, n);
  return resultNormal(valuePtr((void *)dest));
}

/* cmove(dest, src, n) — memmove (aman overlap); return dest.
 * (dulu movepin.) */
static InterpreterResult builtinCmove(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                      Error *error) {
  (void)env;
  const void *src = NULL;
  const void *dest = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "cmove", error, &dest) ||
      !memArgRead(argv[1], false, "cmove", error, &src) ||
      memArgSize(argv[2], "cmove", error) < 0)
    return resultFlow(FLOW_ERROR, valueNull());
  size_t n = (size_t)argv[2].as.number;
  gcmove((void *)dest, src, n);
  return resultNormal(valuePtr((void *)dest));
}

/* cset(ptr, value, n) — memset pada handle; return ptr.
 * (dulu setpin.) */
static InterpreterResult builtinCset(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                     Error *error) {
  (void)env;
  const void *ptr = NULL;
  if (argc < 3 || !memArgRead(argv[0], true, "cset", error, &ptr) ||
      argv[1].type != VALUE_NUMBER || memArgSize(argv[2], "cset", error) < 0)
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

/* Register blok ops — global, tanpa import. Nama contract:
 * cset/cmove/ccpy (dulu setpin/movepin/copypin). */
void rupaMemoryInit(RuntimeEnv *env) {
  if (!env) return;
  semSet(env, "ccpy", valueNativeFunction("ccpy", builtinCcpy, 3));
  semSet(env, "cmove", valueNativeFunction("cmove", builtinCmove, 3));
  semSet(env, "cset", valueNativeFunction("cset", builtinCset, 3));
  semSet(env, "pincmp", valueNativeFunction("pincmp", builtinPincmp, 3));
  // alokasi string (batch 2)
  semSet(env, "dupin", valueNativeFunction("dupin", builtinDupin, 1));
  semSet(env, "maxdupin", valueNativeFunction("maxdupin", builtinMaxdupin, 2));
}
