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

static struct StructLayout *layoutFind(const char *name) {
  if (!name) return NULL;
  for (struct StructLayout *l = g_layouts; l; l = l->next)
    if (!strcmp(l->name, name)) return l;
  return NULL;
}

bool analyzerDeclareStruct(const char *name, const struct StructField *fields,
                           int count) {
  if (!name || !*name) return false;

  struct StructLayout *existing = layoutFind(name);
  if (existing) {
    /* Redeclare: ganti layout (GC akan mengevop yang lama). */
    existing->fields = NULL;
    existing->count = 0;
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

/* Scalar: number/string/boolean/decimal/array/object/function/null. */
static bool isScalarType(const char *type) {
  if (!type) return true;
  return !strcmp(type, "number") || !strcmp(type, "decimal") ||
         !strcmp(type, "string") || !strcmp(type, "boolean") ||
         !strcmp(type, "array") || !strcmp(type, "object") ||
         !strcmp(type, "null") || !strcmp(type, "function");
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

  /* Field tambahan di luar kontrak juga ditolak. */
  for (struct RuntimeObjectEntry *e = value.as.object.entries; e; e = e->next) {
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
