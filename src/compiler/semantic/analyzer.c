#include <rupa.h>

/* Semantic analyzer: struct-first typing (design/next_struct.txt).
 *
 * Deklarasi struct (`Circuit { x: number ... }`) mendaftarkan layout ke
 * registry proses-wide. Validasi annotation (`c: Circuit = {...}`) lalu
 * memeriksa object terhadap kontrak itu — rekursif untuk field bertipe
 * struct dan elemen array-of-struct (`Circuit[]`).
 *
 * Semua alokasi via gcmall/gcstrdup (GC), tidak ada free manual. */

struct StructLayout {
  char *name;
  struct StructField *fields;
  int count;
  bool isClass; /* true = dideklarasi via NODE_CLASS_DECL (bisa di-instantiate) */
  char *parent; /* nama class induk (extends), NULL jika tidak ada */
  struct StructLayout *next;
};

static struct StructLayout *g_layouts = NULL;

void analyzerReset(void) { g_layouts = NULL; }

bool analyzerFindStruct(const char *name) {
  if (!name) return false;
  for (struct StructLayout *l = g_layouts; l; l = l->next)
    if (!strcmp(l->name, name)) return true;
  return false;
}

/* Nama ini class terdaftar (NODE_CLASS_DECL)? Dipakai dispatch
 * instantiation `new ClassName(args)` — hanya class yang bisa
 * di-instantiate, struct murni data tidak. */
bool analyzerIsClass(const char *name) {
  if (!name) return false;
  for (struct StructLayout *l = g_layouts; l; l = l->next)
    if (!strcmp(l->name, name)) return l->isClass;
  return false;
}

/* Nama class induk (extends) — NULL bila tidak ada / bukan class.
 * Dipakai super dispatch dan pewarisan method instance. */
const char *analyzerClassParent(const char *name) {
  if (!name) return NULL;
  for (struct StructLayout *l = g_layouts; l; l = l->next)
    if (!strcmp(l->name, name)) return l->isClass ? l->parent : NULL;
  return NULL;
}

static struct StructLayout *layoutFind(const char *name) {
  if (!name) return NULL;
  for (struct StructLayout *l = g_layouts; l; l = l->next)
    if (!strcmp(l->name, name)) return l;
  return NULL;
}

bool analyzerDeclareStruct(const char *name, const struct StructField *fields,
                           int count) {
  return analyzerDeclareType(name, fields, count, false);
}

/* Varian untuk NODE_CLASS_DECL — menandai layout sebagai class
 * (bisa di-instantiate via `Name({...})`). */
bool analyzerDeclareClass(const char *name, const struct StructField *fields,
                          int count) {
  return analyzerDeclareType(name, fields, count, true);
}

bool analyzerDeclareType(const char *name, const struct StructField *fields,
                         int count, bool isClass) {
  return analyzerDeclareTypeExt(name, fields, count, isClass, NULL);
}

bool analyzerDeclareClassExt(const char *name, const struct StructField *fields,
                             int count, const char *parent) {
  return analyzerDeclareTypeExt(name, fields, count, true, parent);
}

bool analyzerDeclareTypeExt(const char *name, const struct StructField *fields,
                            int count, bool isClass, const char *parent) {
  if (!name || !*name) return false;

  struct StructLayout *existing = layoutFind(name);
  if (existing) {
    /* Redeclare: ganti layout (GC akan mengevop yang lama). */
    existing->fields = NULL;
    existing->count = 0;
    existing->isClass = isClass;
    existing->parent = parent && *parent ? gcstrdup(parent) : NULL;
    if (fields && count > 0) {
      struct StructField *copy = gcmall(sizeof(*copy) * (size_t)count);
      if (!copy) return false;
      for (int i = 0; i < count; i++) {
        copy[i].name = gcstrdup(fields[i].name);
        copy[i].type = gcstrdup(fields[i].type);
      }
      existing->fields = copy;
      existing->count = count;
    }
    return true;
  }

  struct StructLayout *l = gcmall(sizeof(*l));
  if (!l) return false;

  l->name = gcstrdup(name);
  l->fields = NULL;
  l->count = 0;
  l->isClass = isClass;
  l->parent = parent && *parent ? gcstrdup(parent) : NULL;
  if (fields && count > 0) {
    struct StructField *copy = gcmall(sizeof(*copy) * (size_t)count);
    if (!copy) return false;
    for (int i = 0; i < count; i++) {
      copy[i].name = gcstrdup(fields[i].name);
      copy[i].type = gcstrdup(fields[i].type);
    }
    l->fields = copy;
    l->count = count;
  }

  l->next = g_layouts;
  g_layouts = l;
  return true;
}

/* Scalar: number/string/boolean/decimal/array/object/function/null/ptr/void
 * + unknown (value apa pun — tipe data tidak diketahui; dipakai mis. untuk
 * args variadik `construct(args: unknown[])` atau kontrak "terima apa pun").
 * void hanya sah sebagai return-type annotation — matchesScalar menolak
 * value apa pun untuk void (fungsi void tidak mengembalikan nilai). */
static bool isScalarType(const char *type) {
  if (!type) return true;
  return !strcmp(type, "number") || !strcmp(type, "decimal") ||
         !strcmp(type, "string") || !strcmp(type, "boolean") ||
         !strcmp(type, "array") || !strcmp(type, "object") ||
         !strcmp(type, "null") || !strcmp(type, "function") ||
         !strcmp(type, "ptr") || !strcmp(type, "void") ||
         !strcmp(type, "unknown");
}

/* Tipe selain scalar bawaan harus struct terdaftar — kalau tidak,
 * referensi type tak dikenal (typo, belum dideklarasi) = error.
 * Bentuk array "T[]" dicek lewat elemennya. */
static bool isKnownType(const char *type) {
  if (!type) return true;
  size_t length = strlen(type);
  if (length >= 2 && type[length - 2] == '[' && type[length - 1] == ']') {
    char element[128];
    if (length - 2 >= sizeof(element)) return true;
    memcpy(element, type, length - 2);
    element[length - 2] = '\0';
    return isKnownType(element);
  }
  return isScalarType(type) || layoutFind(type) != NULL;
}

bool analyzerIsKnownType(const char *type) { return isKnownType(type); }

static bool matchesScalar(const char *type, RuntimeValue value) {
  if (!type) return true;
  /* void tidak punya nilai — value apa pun (termasuk null) ditolak
   * untuk anotasi `x: void = ...` (void bukan tipe variable). */
  if (!strcmp(type, "void")) return false;
  if (!strcmp(type, "number"))
    return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL;
  if (!strcmp(type, "decimal")) return value.type == VALUE_DECIMAL;
  if (!strcmp(type, "string")) return value.type == VALUE_STRING;
  if (!strcmp(type, "boolean")) return value.type == VALUE_BOOLEAN;
  if (!strcmp(type, "array")) return value.type == VALUE_ARRAY;
  if (!strcmp(type, "object")) return value.type == VALUE_OBJECT;
  if (!strcmp(type, "null")) return value.type == VALUE_NULL;
  if (!strcmp(type, "function"))
    return value.type == VALUE_FUNCTION || value.type == VALUE_NATIVE_FUNCTION;
  if (!strcmp(type, "ptr")) return value.type == VALUE_PTR || value.type == VALUE_NULL;
  /* unknown: tipe data tidak diketahui — menerima value apa pun
   * (termasuk null). Kontrak lemah, dipakai untuk args variadik. */
  if (!strcmp(type, "unknown")) return true;
  return true; /* tipe tak dikenal: biarkan (behavior lama) */
}

static bool checkTypeValue(const char *type, RuntimeValue value, Error *error,
                           int depth);

/* Object vs layout struct. Field hilang/tambahan/tipe-salah = error. */
bool analyzerCheckStruct(const char *name, RuntimeValue value, Error *error) {
  if (!name) return true;

  struct StructLayout *l = layoutFind(name);
  if (!l) {
    /* Struct tidak pernah dideklarasi: error kontrak tak dikenal. */
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, name, "unknown struct type");
    return false;
  }

  if (value.type != VALUE_OBJECT) {
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, name, valueTypeName(value.type));
    return false;
  }

  /* Periksa tiap field kontrak terhadap isi object. */
  for (int i = 0; i < l->count; i++) {
    struct RuntimeObjectEntry *e = value.as.object.entries;
    while (e && strcmp(e->key, l->fields[i].name))
      e = e->next;

    if (!e) {
      if (error) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "missing field %s", l->fields[i].name);
        addRuntimeError(error, ERR_TYPE_MISMATCH, name, buffer);
      }
      return false;
    }

    if (!checkTypeValue(l->fields[i].type, e->value, error, 0)) {
      if (error && error->size == 0)
        addRuntimeError(error, ERR_TYPE_MISMATCH, l->fields[i].type,
                        valueTypeName(e->value.type));
      return false;
    }
  }

  /* Field tambahan di luar kontrak juga ditolak. Anchor implisit
   * (key "") dari object kosong di-skip — metadata internal list. */
  for (struct RuntimeObjectEntry *e = value.as.object.entries; e; e = e->next) {
    if (e->key && e->key[0] == '\0') continue;
    bool known = false;
    for (int i = 0; i < l->count; i++) {
      if (!strcmp(e->key, l->fields[i].name)) {
        known = true;
        break;
      }
    }
    if (!known) {
      if (error) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "unknown field %s", e->key);
        addRuntimeError(error, ERR_TYPE_MISMATCH, name, buffer);
      }
      return false;
    }
  }

  return true;
}

static bool checkTypeValue(const char *type, RuntimeValue value, Error *error,
                           int depth) {
  if (!type || depth > 16) return true;

  size_t length = strlen(type);
  if (length >= 2 && type[length - 2] == '[' && type[length - 1] == ']') {
    if (value.type != VALUE_ARRAY) {
      if (error)
        addRuntimeError(error, ERR_TYPE_MISMATCH, type, valueTypeName(value.type));
      return false;
    }

    char element[128];
    if (length - 2 >= sizeof(element)) return true;
    memcpy(element, type, length - 2);
    element[length - 2] = '\0';

    for (int i = 0; i < value.as.array.length; i++) {
      if (!checkTypeValue(element, value.as.array.items[i], error, depth + 1))
        return false;
    }
    return true;
  }

  /* Tipe struct terdaftar → cek kontrak layout. */
  if (layoutFind(type)) return analyzerCheckStruct(type, value, error);

  if (!isKnownType(type)) {
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, type, "unknown struct type");
    return false;
  }

  if (!matchesScalar(type, value)) {
    if (error)
      addRuntimeError(error, ERR_TYPE_MISMATCH, type, valueTypeName(value.type));
    return false;
  }
  return true;
}

bool analyzerCheckType(const char *type, RuntimeValue value, Error *error) {
  return checkTypeValue(type, value, error, 0);
}

void analyzerSetErrorLocation(Node *node, int typeId) {
  if (!node || typeId < 0 || typeId >= node->length) return;
  setRuntimeErrorLocation(node->ast[typeId].line, node->ast[typeId].row);
}

/* Ukuran scalar untuk sizeof — non-scalar return -1. */
static int scalarTypeSize(const char *type) {
  if (!type) return (int)sizeof(void *);
  /* number 64-bit: representasi runtime VALUE_NUMBER = long long. */
  if (!strcmp(type, "number")) return (int)sizeof(long long);
  if (!strcmp(type, "void")) return 0; /* void tidak menyimpan nilai */
  if (!strcmp(type, "decimal")) return (int)sizeof(double);
  if (!strcmp(type, "boolean")) return 1;
  if (!strcmp(type, "string")) return (int)sizeof(char *);
  if (!strcmp(type, "ptr")) return (int)sizeof(void *);
  if (!strcmp(type, "array") || !strcmp(type, "object") ||
      !strcmp(type, "function") || !strcmp(type, "null") ||
      !strcmp(type, "unknown"))
    return (int)sizeof(void *);
  return -1;
}

/* Ukuran representasi struct: jumlah ukuran field-nya. Field struct
 * bertingkat dihitung rekursif; array-of-struct memakai representasi
 * RuntimeArray. Return false bila ada field bertipe tak dikenal.
 * Rekursi aman: forward reference ditolak saat deklarasi. */
bool analyzerStructSizeOf(const char *name, int *outSize) {
  struct StructLayout *l = layoutFind(name);
  if (!l || !outSize) return false;

  int total = 0;
  for (int i = 0; i < l->count; i++) {
    const char *t = l->fields[i].type;
    size_t length = t ? strlen(t) : 0;
    if (length >= 2 && t[length - 2] == '[' && t[length - 1] == ']') {
      total += (int)sizeof(struct RuntimeArray); /* representasi array */
      continue;
    }
    int s = scalarTypeSize(t);
    if (s < 0) {
      int nested = 0;
      if (!analyzerStructSizeOf(t, &nested)) return false;
      s = nested;
    }
    total += s;
  }
  *outSize = total;
  return true;
}

/* ===== Member access pada handle ptr (design/new_memory.txt, C3) =====
 * Layout = urut deklarasi, tanpa padding — konsisten dengan
 * analyzerStructSizeOf yang dipakai new/Contract untuk alokasi. */

/* Offset byte + tipe field dalam struct terdaftar. Return false bila
 * struct/field tidak ada. */
bool analyzerFieldOffset(const char *structName, const char *fieldName, int *outOffset,
                         char *outType, size_t typeCapacity) {
  struct StructLayout *l = layoutFind(structName);
  if (!l || !fieldName) return false;

  int offset = 0;
  for (int i = 0; i < l->count; i++) {
    const char *t = l->fields[i].type;
    if (!strcmp(l->fields[i].name, fieldName)) {
      if (outOffset) *outOffset = offset;
      if (outType && typeCapacity > 0) snprintf(outType, typeCapacity, "%s", t ? t : "");
      return true;
    }
    size_t length = t ? strlen(t) : 0;
    if (length >= 2 && t[length - 2] == '[' && t[length - 1] == ']') {
      offset += (int)sizeof(struct RuntimeArray); /* representasi array */
      continue;
    }
    int s = scalarTypeSize(t);
    if (s < 0) {
      int nested = 0;
      if (!analyzerStructSizeOf(t, &nested)) return false;
      s = nested;
    }
    offset += s;
  }
  return false;
}

/* Tipe field struct ("number", "People", "People[]", ...). Return
 * false bila struct/field tidak dikenal. */
bool analyzerFieldType(const char *structName, const char *fieldName, char *outType,
                       size_t capacity) {
  return analyzerFieldOffset(structName, fieldName, NULL, outType, capacity);
}
