#include <rupa.h>

/* gc.c — registry & alokasi inti GC.
 * Registry v2: parallel arrays {items, sizes, elems} — size disimpan
 * saat alokasi (gcmall/gccalloc/gcrealloc/gcarray) sehingga sizeof
 * handle, realloc selalu-pindah (handle lama pasti dicabut), dan
 * bounds check operasi blok/indexing menjadi mungkin.
 * Utilitas (dup family, memcpy/memset) ada di gc_extra.c. */

struct GarbageCollector *gc = NULL;

void gcinit(int capacity) {
  if (capacity <= 0 || gc) return;

  gc = calloc(1, sizeof(GarbageCollector));
  if (!gc) return;

  gc->items = malloc(capacity * sizeof(void *));
  if (!gc->items) {
    free(gc);
    gc = NULL;
    return;
  }
  gc->sizes = calloc((size_t)capacity, sizeof(size_t));
  gc->elems = calloc((size_t)capacity, sizeof(size_t));
  gc->types = calloc((size_t)capacity, sizeof(char *));
  if (!gc->sizes || !gc->elems || !gc->types) {
    free(gc->sizes);
    free(gc->elems);
    free(gc->types);
    free(gc->items);
    free(gc);
    gc = NULL;
    return;
  }

  gc->capacity = capacity;
  gc->count = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&gc->lock, &attr);
  pthread_mutexattr_destroy(&attr);
}

void *gcrealloc(void *ptr, size_t new_size) {
  if (!gc) return realloc(ptr, new_size);

  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  void *new_ptr = realloc(ptr, new_size);
  if (new_ptr) {
    if (index != -1) {
      gc->items[index] = new_ptr;
      /* Registry v2: ukuran mengikuti realloc; elemen tidak diketahui
       * di sini (pemanggil setel via gcsetelem bila relevan). */
      gc->sizes[index] = new_size;
    } else
      gcreg(new_ptr);
  }
  pthread_mutex_unlock(&gc->lock);
  return new_ptr;
}

void *gccalloc(size_t num, size_t size) {
  void *ptr = gcmall(num * size);
  if (ptr) {
    memset(ptr, 0, num * size);
    gcsetsize(ptr, num * size);
    gcsetelem(ptr, num);
  }
  return ptr;
}

void *gcmall(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) return NULL;

  gcreg(ptr);
  gcsetsize(ptr, size);
  return ptr;
}

void gcreg(void *ptr) {
  if (!gc || !ptr) return;

  pthread_mutex_lock(&gc->lock);
  if (gc->count >= gc->capacity) {
    int new_capacity = gc->capacity * 2;
    void **new_items = realloc(gc->items, new_capacity * sizeof(void *));
    size_t *new_sizes = realloc(gc->sizes, new_capacity * sizeof(size_t));
    size_t *new_elems = realloc(gc->elems, new_capacity * sizeof(size_t));
    char **new_types = realloc(gc->types, new_capacity * sizeof(char *));
    if (!new_items) {
      pthread_mutex_unlock(&gc->lock);
      return;
    }
    gc->items = new_items;
    gc->sizes = new_sizes;
    gc->elems = new_elems;
    gc->types = new_types;
    gc->capacity = new_capacity;
  }
  gc->items[gc->count] = ptr;
  gc->sizes[gc->count] = 0;
  gc->elems[gc->count] = 0;
  gc->types[gc->count] = NULL;
  gc->count++;
  pthread_mutex_unlock(&gc->lock);
}

void gcfree(void *ptr) {
  if (!gc || !ptr) {
    free(ptr);
    return;
  }

  pthread_mutex_lock(&gc->lock);
  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      free(ptr);
      if (gc->types[i]) {
        free(gc->types[i]);
        gc->types[i] = NULL;
      }
      memmove(&gc->items[i], &gc->items[i + 1], (gc->count - i - 1) * sizeof(void *));
      memmove(&gc->sizes[i], &gc->sizes[i + 1], (gc->count - i - 1) * sizeof(size_t));
      memmove(&gc->elems[i], &gc->elems[i + 1], (gc->count - i - 1) * sizeof(size_t));
      memmove(&gc->types[i], &gc->types[i + 1], (gc->count - i - 1) * sizeof(char *));
      gc->count--;
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  pthread_mutex_unlock(&gc->lock);
  free(ptr);
}

void gcremove(void *ptr) {
  if (!gc || !ptr) return;

  pthread_mutex_lock(&gc->lock);
  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      if (gc->types[i]) {
        free(gc->types[i]);
        gc->types[i] = NULL;
      }
      memmove(&gc->items[i], &gc->items[i + 1], (gc->count - i - 1) * sizeof(void *));
      memmove(&gc->sizes[i], &gc->sizes[i + 1], (gc->count - i - 1) * sizeof(size_t));
      memmove(&gc->elems[i], &gc->elems[i + 1], (gc->count - i - 1) * sizeof(size_t));
      memmove(&gc->types[i], &gc->types[i + 1], (gc->count - i - 1) * sizeof(char *));
      gc->count--;
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  pthread_mutex_unlock(&gc->lock);
}

/* ===== Registry v3: nama tipe elemen ===== */

const char *gcregtype(void *ptr) {
  if (!gc || !ptr) return NULL;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  const char *type = index >= 0 ? gc->types[index] : NULL;
  pthread_mutex_unlock(&gc->lock);
  return type;
}

bool gcregsettype(void *ptr, const char *type) {
  if (!gc || !ptr || !type) return false;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  if (index < 0) {
    pthread_mutex_unlock(&gc->lock);
    return false;
  }
  if (gc->types[index]) free(gc->types[index]);
  /* Metadata registry dikelola MANUAL (bukan gcstrdup — gcstrdup
   * meregestrasi hasilnya juga, dan buffer yang sama akan di-free
   * dua kali: sebagai items[] dan sebagai types[]). */
  gc->types[index] = strdup(type);
  pthread_mutex_unlock(&gc->lock);
  return gc->types[index] != NULL;
}

void gcclean(void) {
  if (!gc) return;

  pthread_mutex_lock(&gc->lock);
  if (gc->count > 0 && gc->items) {
    for (int i = 0; i < gc->count; i++) {
      if (gc->items[i]) {
        free(gc->items[i]);
        gc->items[i] = NULL;
      }
      if (gc->types && gc->types[i]) {
        free(gc->types[i]);
        gc->types[i] = NULL;
      }
    }
    free(gc->items);
    gc->items = NULL;
  }
  if (gc->sizes) free(gc->sizes);
  if (gc->elems) free(gc->elems);
  if (gc->types) free(gc->types);
  pthread_mutex_unlock(&gc->lock);

  pthread_mutex_destroy(&gc->lock);
  free(gc);
  gc = NULL;
}

int gcfind(void *ptr) {
  if (!gc || !ptr) return -1;

  /* Called both with and without lock held — use atomic read for count */
  int n = gc->count;
  for (int i = 0; i < n; i++) {
    if (gc->items[i] == ptr) return i;
  }
  return -1;
}

void *gcarray(void *ptr, size_t count, size_t element_size) {
  /* Semantik reallocarray penuh: tolak jika count * element_size
   * overflow alih-alih diam-diam mengalokasikan kekurangan.
   * ptr == NULL -> alokasi baru (terdaftar), selain itu resize
   * dengan memperbarui registry (via gcrealloc). */
  if (count != 0 && element_size > ((size_t)-1) / count) return NULL;
  void *result = gcrealloc(ptr, count * element_size);
  if (result) {
    gcsetsize(result, count * element_size);
    gcsetelem(result, count);
  }
  return result;
}

/* ===== Registry v2: ukuran blok & elemen ===== */

size_t gcsize(void *ptr) {
  if (!gc || !ptr) return 0;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  size_t size = index >= 0 ? gc->sizes[index] : 0;
  pthread_mutex_unlock(&gc->lock);
  return size;
}

size_t gcelem(void *ptr) {
  if (!gc || !ptr) return 0;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  size_t elem = index >= 0 ? gc->elems[index] : 0;
  pthread_mutex_unlock(&gc->lock);
  return elem;
}

void gcsetsize(void *ptr, size_t size) {
  if (!gc || !ptr) return;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  if (index >= 0) gc->sizes[index] = size;
  pthread_mutex_unlock(&gc->lock);
}

void gcsetelem(void *ptr, size_t count) {
  if (!gc || !ptr) return;
  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  if (index >= 0) gc->elems[index] = count;
  pthread_mutex_unlock(&gc->lock);
}

void *gcresize(void *ptr, size_t old_size, size_t new_size) {
  void *new_ptr = gcrealloc(ptr, new_size);
  if (new_ptr && new_size > old_size) memset((char *)new_ptr + old_size, 0, new_size - old_size);
  return new_ptr;
}
