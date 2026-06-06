#pragma once

#if defined(RUPA_PACKAGE_H)

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
void **gcarray(size_t count, size_t element_size);
void *gcresize(void *ptr, size_t old_size, size_t new_size);

#endif
