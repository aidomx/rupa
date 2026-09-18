#include <rupa.h>

/* gc_extra.c — utilitas GC tambahan.
 *
 * gc.c hanya menampung fungsi utama: registry & alokasi (gcmall,
 * gccalloc, gcrealloc, gcarray, gcfree, gcreg, ...). File ini menampung
 * sisanya:
 *   - dup family (gcstrdup, gcstrndup, ...): alokasi string baru yang
 *     otomatis terdaftar ke registry (via gcmall)
 *   - operasi blok (gccpy, gcset): bekerja pada memori yang sudah ada,
 *     tidak menyentuh registry sama sekali.
 */

/* strdup: duplikasi string (NUL-terminated) ke memori terdaftar GC. */
char *gcstrdup(const char *str) {
  if (!str) return NULL;

  size_t len = strlen(str) + 1;
  char *dup = gcmall(len);
  if (dup) memcpy(dup, str, len);
  return dup;
}

// short gcstrdup
char *gcdup(const char *str) {
  return gcstrdup(str);
}

/* strndup: duplikasi maksimal n karakter, selalu NUL-terminated. */
char *gcstrndup(const char *str, size_t n) {
  if (!str) return NULL;

  char *dup = gcmall(n + 1);
  if (dup) {
    memcpy(dup, str, n);
    dup[n] = '\0';
  }
  return dup;
}

// short gcstrndup
char *gcndup(const char *str, size_t n) {
  return gcstrndup(str, n);
}

/* memcpy pada blok yang sudah ada (tidak menyentuh registry). */
void gccpy(void *dest, const void *src, size_t n) {
  if (!dest || !src || n <= 0) return;
  memcpy(dest, src, n);
}

/* memset pada blok yang sudah ada (tidak menyentuh registry). */
void *gcset(void *dest, int value, size_t n) {
  if (!dest || n <= 0) return dest;
  return memset(dest, value, n);
}

/* memmove pada blok yang sudah ada — aman bila area overlap
 * (tidak menyentuh registry). */
void *gcmove(void *dest, const void *src, size_t n) {
  if (!dest || !src || n <= 0) return dest;
  return memmove(dest, src, n);
}

/* memcmp pada dua blok (read-only): selisih byte pertama yang berbeda,
 * 0 bila identik. */
int gccmp(const void *a, const void *b, size_t n) {
  if (!a || !b || n <= 0) return 0;
  return memcmp(a, b, n);
}
