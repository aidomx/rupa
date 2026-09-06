#include <rupa.h>

/* Manifest functions are now handled by install.c and loader.c.
 * This file is kept for backward compatibility but is no longer used. */

int manifestAddPackage(const char *sourcePath, const char *packageName,
                       const char *version, const char *description) {
  (void)sourcePath;
  (void)packageName;
  (void)version;
  (void)description;
  return -1; /* Not implemented - use install.c */
}

int manifestRemovePackage(const char *packageName) {
  (void)packageName;
  return -1; /* Not implemented - use install.c */
}

int manifestListPackages(void) {
  return -1; /* Not implemented - use install.c */
}
