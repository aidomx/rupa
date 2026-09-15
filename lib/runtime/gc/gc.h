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
  int capacity;         // Batas maksimum referensi yang dapat ditampung
  int count;            // Jumlah referensi aktif dalam konteks ini
  pthread_mutex_t lock; // Mutex for thread-safe allocation
};

extern struct GarbageCollector *gc;

// Initialization
void gcinit(int capacity);
void gcclean(void);

// API functions
void *gcmall(size_t size);
void *gccalloc(size_t num, size_t size);
void *gcrealloc(void *ptr, size_t new_size);
void gcfree(void *ptr);
void gcreg(void *ptr);
void gcremove(void *ptr);
int gcfind(void *ptr);

// Utility functions
char *gcstrdup(const char *str);
char *gcstrndup(const char *str, size_t n);
// short
char *gcdup(const char *str);
char *gcndup(const char *str, size_t n);
void gccpy(void *dest, const void *src, size_t n);
void **gcarray(size_t count, size_t element_size);
void *gcresize(void *ptr, size_t old_size, size_t new_size);

#endif
