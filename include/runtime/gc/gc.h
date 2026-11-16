#pragma once
#if defined(RUPA_PACKAGE_H)
/**
 * @struct GarbageCollector
 * @brief Representasi konteks pengelolaan memori pada runtime.
 *
 * GarbageCollector merepresentasikan *state konseptual* dari kepemilikan
 * memori selama eksekusi program. Setiap alokasi yang relevan terhadap
 * runtime (AST node, token, string, object sementara, dsb.) dapat
 * diregistrasikan ke dalam struktur ini.
 *
 * Struktur ini tidak menentukan *kapan* atau *bagaimana* memori dibebaskan,
 * melainkan hanya menjamin bahwa seluruh alokasi yang terdaftar
 * memiliki titik kontrol yang sama dan dapat diinspeksi oleh sistem
 * (parser, interpreter, semantic analyzer, REPL).
 *
 * Dalam konteks bahasa:
 * - GarbageCollector bertindak sebagai "root" kepemilikan memori
 * - Seluruh fase (lexing → parsing → runtime) dapat berbagi konteks ini
 * - Error, interrupt, atau akhir scope dapat memicu pembersihan terpusat
 *
 * Dengan pendekatan ini, manajemen memori menjadi deterministik,
 * dapat diprediksi, dan mudah di-debug tanpa bergantung pada GC otomatis.
 */
struct GarbageCollector {
  void **items;         // Daftar referensi alokasi yang berada dalam konteks runtime
  size_t *sizes;        // Registry v2: ukuran blok per alokasi (bytes)
  size_t *elems;        // Registry v2: jumlah elemen per alokasi (calloc/reallocarray)
  char **types;         // Registry v3: nama tipe elemen per alokasi (new/Contract)
  char **vheads;        // Registry v4: kepala rantai view per slot owner (gc.c)
  int capacity;         // Batas maksimum referensi yang dapat ditampung
  int count;            // Jumlah referensi aktif dalam konteks ini
  pthread_mutex_t lock; // Mutex for thread-safe allocation
};

extern struct GarbageCollector *gc;

// Initialization
void gcinit(int capacity);
void gcclean(void);

// API functions — registry & alokasi (gc.c)
void *gcmall(size_t size);
void *gccalloc(size_t num, size_t size);
void *gcrealloc(void *ptr, size_t new_size);
void gcfree(void *ptr);
void gcreg(void *ptr);
void gcremove(void *ptr);
int gcfind(void *ptr);
/* Semantik reallocarray(ptr, count, size): NULL jika count * size
 * overflow; resize (atau alokasi baru jika ptr NULL) dengan registry
 * GC tetap mengikuti. */
void *gcarray(void *ptr, size_t count, size_t element_size);
void *gcresize(void *ptr, size_t old_size, size_t new_size);

/* Registry v2 — ukuran blok & elemen (gc.c). Size terisi otomatis oleh
 * gcmall/gccalloc/gcrealloc/gcarray; elemen hanya oleh gccalloc/gcarray.
 * gcsize/gcelem return 0 bila ptr tidak terdaftar. */
size_t gcsize(void *ptr);
size_t gcelem(void *ptr);
void gcsetsize(void *ptr, size_t size);
void gcsetelem(void *ptr, size_t count);

/* Registry v3 — nama tipe elemen (new T()/Contract). String didup via
 * gcstrdup; gcregtype return true bila tercatat. */
const char *gcregtype(void *ptr);
bool gcregsettype(void *ptr, const char *type);

/* View handle (memory.c) — blok GC kecil {owner, offset} yang menunjuk
 * ke dalam blok struct lain (field nested, elemen array-of-struct).
 * Bukan mekanisme registry: view = item biasa bertipe struct tujuan,
 * ditandai elems == GC_VIEW_MAGIC. gcfree me-NULL-kan owner view yang
 * menunjuk blok yang dibebaskan. */
#define GC_VIEW_MAGIC ((size_t)0x52505056) /* "RPPV" */

void *gcregview(void *owner, size_t offset);
void *gcregisview(void *addr);

// Utility functions (gc_extra.c)
char *gcstrdup(const char *str);
char *gcstrndup(const char *str, size_t n);
// short
char *gcdup(const char *str);
char *gcndup(const char *str, size_t n);
void gccpy(void *dest, const void *src, size_t n);
void *gcset(void *dest, int value, size_t n);
void *gcmove(void *dest, const void *src, size_t n);
int gccmp(const void *a, const void *b, size_t n);

#endif
