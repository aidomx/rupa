#include <rupa.h>

struct GarbageCollector *gc = NULL;

/**
 * @brief Inisialisasi garbage collector
 *
 * @param capacity Kapasitas awal
 */
void gcinit(int capacity) {
  if (capacity <= 0 || gc)
    return;

  gc = calloc(1, sizeof(GarbageCollector));
  if (!gc)
    return;

  gc->items = malloc(capacity * sizeof(void *));
  if (!gc->items) {
    free(gc);
    gc = NULL;
    return;
  }

  gc->capacity = capacity;
  gc->count = 0;
}

/**
 * @brief Reallocate memory dengan GC tracking
 */
void *gcrealloc(void *ptr, size_t new_size) {
  if (!gc)
    return realloc(ptr, new_size);

  // Cari index pointer lama
  int index = gcfind(ptr);

  // Reallocate memory
  void *new_ptr = realloc(ptr, new_size);
  if (!new_ptr)
    return NULL;

  // Update pointer di GC jika found
  if (index != -1) {
    gc->items[index] = new_ptr;
  } else {
    // Register new pointer jika tidak ditemukan
    gcreg(new_ptr);
  }

  return new_ptr;
}

/**
 * @brief Allocate dan zero-initialize memory
 */
void *gccalloc(size_t num, size_t size) {
  void *ptr = gcmall(num * size);
  if (ptr) {
    memset(ptr, 0, num * size);
  }
  return ptr;
}

/**
 * @brief Allocate memory dengan GC tracking
 */
void *gcmall(size_t size) {
  void *ptr = malloc(size);
  if (!ptr)
    return NULL;

  gcreg(ptr);
  return ptr;
}

/**
 * @brief Register pointer ke GC
 */
void gcreg(void *ptr) {
  if (!gc || !ptr)
    return;

  // Check jika perlu resize
  if (gc->count >= gc->capacity) {
    int new_capacity = gc->capacity * 2;
    void **new_items = realloc(gc->items, new_capacity * sizeof(void *));
    if (!new_items)
      return;

    gc->items = new_items;
    gc->capacity = new_capacity;
  }

  gc->items[gc->count++] = ptr;
}

/**
 * @brief Free memory dan remove dari GC
 */
void gcfree(void *ptr) {
  if (!gc || !ptr) {
    free(ptr);
    return;
  }

  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      free(ptr);
      // Remove dari array
      memmove(&gc->items[i], &gc->items[i + 1],
              (gc->count - i - 1) * sizeof(void *));
      gc->count--;
      return;
    }
  }

  // Jika tidak ditemukan di GC, free biasa
  free(ptr);
}

/**
 * @brief Remove pointer dari GC tanpa free memory
 */
void gcremove(void *ptr) {
  if (!gc || !ptr)
    return;

  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      memmove(&gc->items[i], &gc->items[i + 1],
              (gc->count - i - 1) * sizeof(void *));
      gc->count--;
      return;
    }
  }
}

/**
 * @brief Clean semua memory yang terdaftar
 */
void gcclean(void) {
  if (!gc)
    return;

  if (gc->count > 0 && gc->items) {
    for (int i = 0; i < gc->count; i++) {
      if (gc->items[i]) {
        free(gc->items[i]);
        gc->items[i] = NULL;
      }
    }

    free(gc->items);
    gc->items = NULL;
  }

  free(gc);
  gc = NULL;
}

/**
 * @brief Find index pointer dalam GC
 */
int gcfind(void *ptr) {
  if (!gc || !ptr)
    return -1;

  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Duplicate string dengan GC
 */
char *gcstrdup(const char *str) {
  if (!str)
    return NULL;

  size_t len = strlen(str) + 1;
  char *dup = gcmall(len);
  if (dup) {
    memcpy(dup, str, len);
  }
  return dup;
}

/**
 * @brief Duplicate string dengan length tertentu
 */
char *gcstrndup(const char *str, size_t n) {
  if (!str)
    return NULL;

  char *dup = gcmall(n + 1);
  if (dup) {
    memcpy(dup, str, n);
    dup[n] = '\0';
  }
  return dup;
}

/**
 * @brief Allocate array of pointers dengan GC
 */
void **gcarray(size_t count, size_t element_size) {
  return gccalloc(count, element_size);
}

/**
 * @brief Resize array dengan GC
 */
void *gcresize(void *ptr, size_t old_size, size_t new_size) {
  void *new_ptr = gcrealloc(ptr, new_size);
  if (new_ptr && new_size > old_size) {
    // Zero-initialize new portion
    memset((char *)new_ptr + old_size, 0, new_size - old_size);
  }
  return new_ptr;
}
