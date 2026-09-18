#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * Satu field dalam layout struct: nama field + nama tipenya.
 * Tipe berupa string ("number", "Circuit", "Circuit[]", ...).
 */
struct StructField {
  char *name;
  char *type;
};

/**
 * Registry layout struct — dikontrak oleh NODE_STRUCT_DECL, dibaca oleh
 * validasi annotation (interpretAssign / interpretAnnotation / IR check).
 *
 * Design: struct-first typing (design/next_struct.txt) — deklarasi struct
 * jadi kontrak bentuk, annotation `c: Circuit` memeriksa object terhadap
 * layout-nya secara rekursif (termasuk array-of-struct `Circuit[]`).
 */

/* Reset registry — dipanggil sekali per file (script/test/REPL) agar
 * deklarasi struct tidak bocor lintas program. */
void analyzerReset(void);

/* Daftarkan/mutakhirkan layout struct: name + daftar field (nama:tipe).
 * Semua string didup via gcstrdup (dikelola GC, tanpa free manual). */
bool analyzerDeclareStruct(const char *name, const struct StructField *fields,
                           int count);
bool analyzerDeclareClass(const char *name, const struct StructField *fields,
                          int count);
bool analyzerDeclareType(const char *name, const struct StructField *fields,
                         int count, bool isClass);

/* Nama ini class terdaftar (NODE_CLASS_DECL)? Dispatch instantiation
 * `c = Counter({...})` hanya untuk class, bukan struct murni. */
bool analyzerIsClass(const char *name);
const char *analyzerClassParent(const char *name); /* NULL bila tanpa extends */
bool analyzerDeclareClassExt(const char *name, const struct StructField *fields,
                             int count, const char *parent);
bool analyzerDeclareTypeExt(const char *name, const struct StructField *fields,
                            int count, bool isClass, const char *parent);

/* Struct terdaftar dengan nama ini? */
bool analyzerFindStruct(const char *name);

/* Tipe dikenal? (scalar bawaan atau struct terdaftar). Dipakai saat
 * deklarasi struct untuk menolak referensi type tak dikenal. */
bool analyzerIsKnownType(const char *type);
/* Validasi value terhadap nama tipe (bisa berupa "Struct", "Struct[]",
 * atau scalar number/string/boolean/...). Return false + tambah error
 * ERR_TYPE_MISMATCH bila bentuk tidak cocok. */
bool analyzerCheckType(const char *type, RuntimeValue value, Error *error);

/* Validasi object terhadap layout struct (rekursif untuk field bertipe
 * struct / array-of-struct). */
bool analyzerCheckStruct(const char *name, RuntimeValue value, Error *error);

/* Ukuran representasi struct terdaftar (jumlah ukuran field, rekursif
 * untuk struct bertingkat). Return false bila tak terdaftar. Dipakai
 * rupamemorySizeOf untuk sizeof(Struct). */
bool analyzerStructSizeOf(const char *name, int *outSize);

/* Member access pada handle ptr (design/new_memory.txt, C3): offset
 * byte + tipe field dalam struct terdaftar (layout = urut deklarasi,
 * tanpa padding — konsisten dengan analyzerStructSizeOf). */
bool analyzerFieldOffset(const char *structName, const char *fieldName, int *outOffset,
                         char *outType, size_t typeCapacity);
bool analyzerFieldType(const char *structName, const char *fieldName, char *outType,
                       size_t capacity);

/* Tempel posisi source (line/row dari AST node) ke error runtime yang
 * akan dibuat — agar TypeError menunjuk baris yang benar. */
void analyzerSetErrorLocation(Node *node, int typeId);

#endif
