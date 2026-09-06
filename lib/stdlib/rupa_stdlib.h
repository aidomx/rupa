#pragma once

#include "io.h"
#include "os.h"
#include "string.h"

#if defined(RUPA_PACKAGE_H)

/* Initialize all standard modules and register them in the environment */
void stdlibInit(RuntimeEnv *env);

/* Get a standard module by name */
bool stdlibGetModule(const char *name, RuntimeValue *out);

/* Module management: add, update, delete, list */
int stdlibManage(const char *args[], int length);

/* Initialize stdlib loader — scan directories and cache module paths */
void stdlibLoaderInit(void);

/* Find a stdlib module by name (returns file path or NULL) */
const char *stdlibFindModule(const char *name);

/* Get the count of cached stdlib modules */
int stdlibModuleCount(void);

/* Refresh stdlib cache after adding/removing packages */
void stdlibLoaderRefresh(void);

/* Get the path to ~/.rupa/rupa_modules.tar.gz (global archive)
 * Caller must free() the returned string. */
char *getGlobalArchive(void);

/* Cleanup extracted stdlib directory */
void stdlibLoaderCleanup(void);

/* Manifest operations */
int manifestAddPackage(const char *sourcePath, const char *packageName,
                       const char *version, const char *description);
int manifestRemovePackage(const char *packageName);
int manifestListPackages(void);

#endif
