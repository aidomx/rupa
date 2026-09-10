#include <rupa.h>

struct GarbageCollector *gc = NULL;

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
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&gc->lock, &attr);
  pthread_mutexattr_destroy(&attr);
}

void *gcrealloc(void *ptr, size_t new_size) {
  if (!gc)
    return realloc(ptr, new_size);

  pthread_mutex_lock(&gc->lock);
  int index = gcfind(ptr);
  void *new_ptr = realloc(ptr, new_size);
  if (new_ptr) {
    if (index != -1)
      gc->items[index] = new_ptr;
    else
      gcreg(new_ptr);
  }
  pthread_mutex_unlock(&gc->lock);
  return new_ptr;
}

void *gccalloc(size_t num, size_t size) {
  void *ptr = gcmall(num * size);
  if (ptr)
    memset(ptr, 0, num * size);
  return ptr;
}

void *gcmall(size_t size) {
  void *ptr = malloc(size);
  if (!ptr)
    return NULL;

  gcreg(ptr);
  return ptr;
}

void gcreg(void *ptr) {
  if (!gc || !ptr)
    return;

  pthread_mutex_lock(&gc->lock);
  if (gc->count >= gc->capacity) {
    int new_capacity = gc->capacity * 2;
    void **new_items = realloc(gc->items, new_capacity * sizeof(void *));
    if (!new_items) {
      pthread_mutex_unlock(&gc->lock);
      return;
    }
    gc->items = new_items;
    gc->capacity = new_capacity;
  }
  gc->items[gc->count++] = ptr;
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
      memmove(&gc->items[i], &gc->items[i + 1],
              (gc->count - i - 1) * sizeof(void *));
      gc->count--;
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  pthread_mutex_unlock(&gc->lock);
  free(ptr);
}

void gcremove(void *ptr) {
  if (!gc || !ptr)
    return;

  pthread_mutex_lock(&gc->lock);
  for (int i = 0; i < gc->count; i++) {
    if (gc->items[i] == ptr) {
      memmove(&gc->items[i], &gc->items[i + 1],
              (gc->count - i - 1) * sizeof(void *));
      gc->count--;
      pthread_mutex_unlock(&gc->lock);
      return;
    }
  }
  pthread_mutex_unlock(&gc->lock);
}

void gcclean(void) {
  if (!gc)
    return;

  pthread_mutex_lock(&gc->lock);
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
  pthread_mutex_unlock(&gc->lock);

  pthread_mutex_destroy(&gc->lock);
  free(gc);
  gc = NULL;
}

int gcfind(void *ptr) {
  if (!gc || !ptr)
    return -1;

  /* Called both with and without lock held — use atomic read for count */
  int n = gc->count;
  for (int i = 0; i < n; i++) {
    if (gc->items[i] == ptr)
      return i;
  }
  return -1;
}

char *gcstrdup(const char *str) {
  if (!str)
    return NULL;

  size_t len = strlen(str) + 1;
  char *dup = gcmall(len);
  if (dup)
    memcpy(dup, str, len);
  return dup;
}

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

void **gcarray(size_t count, size_t element_size) {
  return gccalloc(count, element_size);
}

void *gcresize(void *ptr, size_t old_size, size_t new_size) {
  void *new_ptr = gcrealloc(ptr, new_size);
  if (new_ptr && new_size > old_size)
    memset((char *)new_ptr + old_size, 0, new_size - old_size);
  return new_ptr;
}
