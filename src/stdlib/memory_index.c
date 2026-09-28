#include <rupa.h>

/* memory_index.c — indexing VALUE_PTR (x[i] baca/tulis), interpreter
 * dan IR machine. Unit hasil split memory.c:
 *   memory_new.c     — new/Contract (alokasi type-driven)
 *   memory_member.c  — member access struct + string slot
 *   memory_builtin.c — del/dupl/compare + memoryInit
 */

/* Inti value-based — dipakai interpreter (setelah evaluasi target/index)
 * dan IR machine (nilai langsung dari register). Return false + error
 * (dan *fatal = true) bila akses tidak valid. */
static bool memoryPtrAccess(void *ptr, RuntimeValue idx, bool isWrite, RuntimeValue val,
                            RuntimeValue *out, Error *error, bool *fatal) {
  *fatal = false;
  if (!ptr || gcfind(ptr) < 0) {
    if (error) {
      addRuntimeError(error, ERR_MEMORY, isWrite ? "x[i] = v" : "x[i]", "pointer not owned by GC");
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
    snprintf(message, sizeof(message), "index %lld is out of bounds for block of %zu element(s)", i,
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
          addRuntimeError(error, ERR_MEMORY, "x[i]", "cannot create element view (out of memory)");
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
    addRuntimeError(error, ERR_TYPE_MISMATCH, "x[i]", "element type not readable via indexing");
  }
  *fatal = true;
  return false;
}

/* x[i] — baca elemen handle: scalar (i == 0) atau blok T (0 <= i < n). */
InterpreterResult memoryIndexGet(Node *node, int targetId, int indexId, RuntimeEnv *env,
                                 Error *error, bool *handled) {
  *handled = false;
  if (!node || targetId < 0 || targetId >= node->length) return resultNormal(valueNull());

  InterpreterResult target = interpretNode(node, targetId, env, error);
  if (target.flow != FLOW_NORMAL) return target;
  if (target.value.type != VALUE_PTR) return resultNormal(valueNull());

  *handled = true;
  InterpreterResult idx = interpretNode(node, indexId, env, error);
  if (idx.flow != FLOW_NORMAL) return idx;

  RuntimeValue out = valueNull();
  bool fatal = false;
  if (!memoryPtrAccess(target.value.as.ptr, idx.value, false, valueNull(), &out, error, &fatal))
    return resultFlow(FLOW_ERROR, valueNull());
  return resultNormal(out);
}

/* x[i] = v — tulis elemen handle (scalar index 0 atau blok). */
InterpreterResult memoryIndexSet(Node *node, int targetId, int indexId, RuntimeValue val,
                                 RuntimeEnv *env, Error *error, bool *handled) {
  *handled = false;
  if (!node || targetId < 0 || targetId >= node->length) return resultNormal(valueNull());

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

/* Versi value-based untuk IR machine — nilai sudah di register. */
RuntimeValue memoryPtrGet(RuntimeValue ptr, RuntimeValue idx, Error *error, bool *fatal) {
  RuntimeValue out = valueNull();
  if (ptr.type != VALUE_PTR ||
      !memoryPtrAccess(ptr.as.ptr, idx, false, valueNull(), &out, error, fatal))
    return valueNull();
  return out;
}

bool memoryPtrSet(RuntimeValue ptr, RuntimeValue idx, RuntimeValue val, Error *error, bool *fatal) {
  if (ptr.type != VALUE_PTR) return false;
  RuntimeValue out = valueNull();
  return memoryPtrAccess(ptr.as.ptr, idx, true, val, &out, error, fatal);
}
