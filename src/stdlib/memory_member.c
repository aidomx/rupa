#include <rupa.h>

/* memory_member.c — member access struct pada handle ptr (baca/tulis
 * field) + string slot first-class (design/str_memory.txt). Unit hasil
 * split memory.c:
 *   memory_new.c     — new/Contract (alokasi type-driven)
 *   memory_index.c   — indexing VALUE_PTR (get/set interpreter+IR)
 *   memory_builtin.c — del/dupl/compare + memoryInit
 */

/* Baca field struct pada handle ptr: offset dari layout analyzer
 * (C-style: field diselaskan pada alignment-nya), decode sesuai tipe
 * field. string/array = slot/snapshot; struct bertingkat = view handle. */
static bool memberFieldRead(void *ptr, const char *structType, const char *field, RuntimeValue *out,
                            Error *error) {
  int offset = 0;
  char ftype[256];
  if (!analyzerFieldOffset(structType, field, &offset, ftype, sizeof(ftype))) {
    char message[512];
    snprintf(message, sizeof(message), "unknown field '%s' on struct '%s'", field, structType);
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
      addRuntimeError(error, ERR_MEMORY, structType, "cannot create field view (out of memory)");
      return false;
    }
    *out = valuePtr(view);
    return true;
  }

  /* Array-of-struct (C-B2): field "T[]" menampung RuntimeArray
   * berisi view handle elemen (ditulis memberFieldWrite). Baca
   * kembali sebagai VALUE_ARRAY sehingga subscript arr[i] bekerja
   * di atas view handle elemen — bukan snapshot deskriptif. */
  size_t ftypeLen = strlen(ftype);
  if (ftypeLen >= 2 && !strcmp(ftype + ftypeLen - 2, "[]")) {
    struct RuntimeArray array;
    memcpy(&array, src, sizeof(array));
    *out = valueArray(array.items, array.length);
    return true;
  }

  /* Kompleks lain (ptr): buffer scalar hanya menyimpan byte
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
  valueObjectSet(&obj, "note", valueString(gcstrdup("raw field snapshot (buffer-scalar)")));
  *out = obj;
  return true;
}

/* Tulis field struct pada handle ptr: encode sesuai tipe field.
 * number/decimal/boolean ditulis native; tipe kompleks ditolak. */
static bool memberFieldWrite(void *ptr, const char *structType, const char *field, RuntimeValue val,
                             Error *error) {
  int offset = 0;
  char ftype[256];
  if (!analyzerFieldOffset(structType, field, &offset, ftype, sizeof(ftype))) {
    char message[512];
    snprintf(message, sizeof(message), "unknown field '%s' on struct '%s'", field, structType);
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
    if (val.type == VALUE_NULL) {
      char *nullp = NULL;
      memcpy(dest, &nullp, sizeof(nullp));
      return true;
    }
    (void)slotp;
    goto typefail;
  }

  size_t ftypeLen = strlen(ftype);
  if (ftypeLen >= 2 && !strcmp(ftype + ftypeLen - 2, "[]")) {
    if (val.type != VALUE_PTR || !val.as.ptr || gcfind(val.as.ptr) < 0) {
      goto typefail;
    }

    char elemType[256];
    if (ftypeLen - 2 >= sizeof(elemType)) {
      goto typefail;
    }

    memcpy(elemType, ftype, ftypeLen - 2);
    elemType[ftypeLen - 2] = '\0';

    const char *valueType = gcregtype(val.as.ptr);
    if (!valueType || strcmp(valueType, elemType) != 0) {
      goto typefail;
    }

    size_t count = gcelem(val.as.ptr);
    int elemBytes = 0;

    if (count == 0 || !analyzerStructSizeOf(elemType, &elemBytes) || elemBytes <= 0) {
      goto typefail;
    }

    RuntimeValue *items = gccalloc(count, sizeof(*items));
    if (!items) {
      return false;
    }

    for (size_t i = 0; i < count; i++) {
      void *item = gcregview(val.as.ptr, i * (size_t)elemBytes);
      if (!item) {
        return false;
      }

      items[i] = valuePtr(item);
    }

    struct RuntimeArray array = {
        .items = items,
        .length = (int)count,
    };

    memcpy(dest, &array, sizeof(array));
    return true;
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
    snprintf(message, sizeof(message), "field '%s' expects a struct handle/view of '%s'", field,
             ftype);
    addRuntimeError(error, ERR_TYPE_MISMATCH, ftype, message);
    return false;
  }

typefail: {
  char message[512];
  snprintf(message, sizeof(message),
           "field '%s' of type '%s' is not writable via handle (complex field)", field, ftype);
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
  if (!stype || !analyzerFindStruct(stype)) return false; /* bukan struct handle — jalur lama */
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
bool memoryMemberGet(void *ptr, const char *field, RuntimeValue *out, Error *error, bool *fatal) {
  return structMemberAccess(ptr, field, false, valueNull(), out, error, fatal);
}

bool memoryMemberSet(void *ptr, const char *field, RuntimeValue val, Error *error, bool *fatal) {
  RuntimeValue out = valueNull();
  return structMemberAccess(ptr, field, true, val, &out, error, fatal);
}

/* ==================== string slot ==================== */

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
