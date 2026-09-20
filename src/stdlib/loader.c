#include <rupa.h>
/* Maximum number of external stdlib modules */
#define MAX_STDLIB_MODULES 64

/* Cached module entries */
typedef struct {
  char name[128];
  char path[1024];
  int priority;
  bool loaded;
} StdlibModuleEntry;

static StdlibModuleEntry stdlibCache[MAX_STDLIB_MODULES];
static int stdlibCacheCount = 0;

/* Extracted stdlib directory paths */
static char extractedSystemDir[1024] = {0};
static char extractedGlobalDir[1024] = {0};
static char extractedLocalDir[1024] = {0};

/*
 * The embedded system archive is produced by rbot (Buildfile: embedded).
 * rbot generates build/embedded.h with generic per-entry macros — for an
 * entry named `modules` these are EMBED_MODULES_ARCHIVE_NAME, _EXTRACT_DIR,
 * _SYMBOL, _SYMBOL_END. rupa's loader consumes the `modules` entry; the
 * weak externs below are the fallback layout used when the generated header
 * is unavailable (e.g. plain `make` without rbot).
 */
#if __has_include("build/embedded.h")
#include "build/embedded.h"
extern const unsigned char EMBED_MODULES_SYMBOL[] __attribute__((weak));
extern const unsigned char EMBED_MODULES_SYMBOL_END[] __attribute__((weak));
#define RUPA_EMBEDDED_START        EMBED_MODULES_SYMBOL
#define RUPA_EMBEDDED_END          EMBED_MODULES_SYMBOL_END
#define RUPA_EMBEDDED_ARCHIVE_NAME EMBED_MODULES_ARCHIVE_NAME
#define RUPA_EMBEDDED_EXTRACT_DIR  EMBED_MODULES_EXTRACT_DIR
#else
#define RUPA_EMBEDDED_ARCHIVE_NAME "rupa_modules.tar.gz"
#define RUPA_EMBEDDED_EXTRACT_DIR "/tmp/rupa-system"
extern const unsigned char _binary_modules_rupa_modules_tar_gz_start[] __attribute__((weak));
extern const unsigned char _binary_modules_rupa_modules_tar_gz_end[] __attribute__((weak));
#define RUPA_EMBEDDED_START _binary_modules_rupa_modules_tar_gz_start
#define RUPA_EMBEDDED_END   _binary_modules_rupa_modules_tar_gz_end
#endif

/**
 * Get the extraction path for the embedded system archive.
 */
static char *getSystemExtractDir(void) {
  static char dir[1024];
  /* Configured by the build (Buildfile: embedded.extract), e.g. /tmp/rupa-system */
  const char *custom = RUPA_EMBEDDED_EXTRACT_DIR;
  const char *env = getenv("RUPA_EXTRACT_DIR");
  snprintf(dir, sizeof(dir), "%s", (env && *env) ? env : ((custom && *custom) ? custom : ""));
  if (dir[0] == '\0') snprintf(dir, sizeof(dir), "/tmp/rupa-system");
  return dir;
}

/**
 * Get the path to ~/.rupa/rupa_modules.tar.gz (global archive)
 * Caller must free() the returned string.
 */
char *getGlobalArchive(void) {
  const char *home = getenv("HOME");
  if (!home) return NULL;

  size_t len = strlen(home) + strlen("/.rupa/rupa_modules.tar.gz") + 1;
  char *path = malloc(len);
  if (!path) return NULL;
  snprintf(path, len, "%s/.rupa/rupa_modules.tar.gz", home);
  return path;
}

/**
 * Get the path to ./modules/rupa_modules.tar.gz (local archive)
 * Caller must free() the returned string.
 */
static char *getLocalArchive(void) {
  char *path = malloc(1024);
  if (!path) return NULL;
  snprintf(path, 1024, "modules/rupa_modules.tar.gz");
  return path;
}

/**
 * Get the temp extraction directory for global archive.
 */
static char *getGlobalExtractDir(void) {
  static char dir[1024];
  snprintf(dir, sizeof(dir), "/tmp/rupa-global");
  return dir;
}

/**
 * Get the temp extraction directory for local archive.
 */
static char *getLocalExtractDir(void) {
  static char dir[1024];
  snprintf(dir, sizeof(dir), "/tmp/rupa-local");
  return dir;
}

/**
 * Check if a file exists.
 */
static bool fileExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

/**
 * Remove a directory recursively.
 */
static void removeDir(const char *dirpath) {
  DIR *dir = opendir(dirpath);
  if (!dir) return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

    struct stat st;
    if (stat(filepath, &st) != 0) continue;

    if (S_ISDIR(st.st_mode)) {
      removeDir(filepath);
    } else {
      remove(filepath);
    }
  }
  closedir(dir);
  rmdir(dirpath);
}

/**
 * Create directory recursively.
 */
static void mkdirp(const char *path, mode_t mode) {
  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  mkdir(tmp, mode);
}

/* Forward declaration because the embedded archive uses the same extractor. */
static int extractArchive(const char *archivePath, const char *extractDir);

/**
 * Extract the archive embedded in the executable.
 */
static int extractEmbeddedArchive(const char *extractDir) {
  const unsigned char *embStart = RUPA_EMBEDDED_START;
  const unsigned char *embEnd = RUPA_EMBEDDED_END;
  if (!embStart || !embEnd || embEnd <= embStart) return -1;

  char archivePath[1024];
  snprintf(archivePath, sizeof(archivePath), "%s/%s", extractDir, RUPA_EMBEDDED_ARCHIVE_NAME);
  mkdirp(extractDir, 0755);

  FILE *archive = fopen(archivePath, "wb");
  if (!archive) return -1;
  size_t size = (size_t)(embEnd - embStart);
  bool written = fwrite(embStart, 1, size, archive) == size;
  fclose(archive);
  if (!written) return -1;

  char extracted[1024];
  snprintf(extracted, sizeof(extracted), "%s/extracted", extractDir);
  return extractArchive(archivePath, extracted);
}

/**
 * Extract a tar.gz archive to a directory.
 * Returns 0 on success, -1 on failure.
 */
static int extractArchive(const char *archivePath, const char *extractDir) {
  if (!fileExists(archivePath)) return -1;

  /* Remove old extraction */
  if (fileExists(extractDir)) {
    removeDir(extractDir);
  }

  /* Create extraction directory */
  mkdirp(extractDir, 0755);

  /* Extract; plain tar when there is no .gz suffix, tar.gz otherwise */
  size_t alen = strlen(archivePath);
  bool gz = alen > 3 && strcmp(archivePath + alen - 3, ".gz") == 0;
  char cmd[2048];
  snprintf(cmd, sizeof(cmd), gz ? "tar xzf \"%s\" -C \"%s\" 2>/dev/null"
                                : "tar xf \"%s\" -C \"%s\" 2>/dev/null",
           archivePath, extractDir);
  return system(cmd);
}

/**
 * Extract module name from filename.
 * "math.rp" -> "math"
 * Caller must free() the returned string.
 */
static char *moduleNameFromFilename(const char *filename) {
  size_t len = strlen(filename);
  if (len <= 3) return NULL;

  char *name = malloc(len - 2);
  if (!name) return NULL;

  strncpy(name, filename, len - 3);
  name[len - 3] = '\0';
  return name;
}

/**
 * Check if a file is a .rp file.
 */
static bool isRpFile(const char *name) {
  size_t len = strlen(name);
  return len > 3 && strcmp(name + len - 3, ".rp") == 0;
}

/* Add a module to the cache. Higher-priority sources override lower ones. */
static void cacheAdd(const char *name, const char *path, int priority) {
  if (stdlibCacheCount >= MAX_STDLIB_MODULES) return;

  for (int i = 0; i < stdlibCacheCount; i++) {
    if (strcmp(stdlibCache[i].name, name) != 0) continue;
    if (priority > stdlibCache[i].priority) {
      strncpy(stdlibCache[i].path, path, sizeof(stdlibCache[i].path) - 1);
      stdlibCache[i].path[sizeof(stdlibCache[i].path) - 1] = '\0';
      stdlibCache[i].priority = priority;
      stdlibCache[i].loaded = false;
    }
    return;
  }

  strncpy(stdlibCache[stdlibCacheCount].name, name, sizeof(stdlibCache[stdlibCacheCount].name) - 1);
  stdlibCache[stdlibCacheCount].name[sizeof(stdlibCache[stdlibCacheCount].name) - 1] = '\0';
  strncpy(stdlibCache[stdlibCacheCount].path, path, sizeof(stdlibCache[stdlibCacheCount].path) - 1);
  stdlibCache[stdlibCacheCount].path[sizeof(stdlibCache[stdlibCacheCount].path) - 1] = '\0';
  stdlibCache[stdlibCacheCount].priority = priority;
  stdlibCache[stdlibCacheCount].loaded = false;
  stdlibCacheCount++;
}

/**
 * Scan a directory for .rp files and subdirectories.
 * - Top-level .rp files -> cached as module name
 * - Subdirectories with index.rp -> cached as dir name
 */
static void scanDir(const char *dirpath, int priority) {
  DIR *dir = opendir(dirpath);
  if (!dir) return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (stdlibCacheCount >= MAX_STDLIB_MODULES) break;

    /* Skip . and .. */
    if (entry->d_name[0] == '.') continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

    struct stat st;
    if (stat(filepath, &st) != 0) continue;

    if (S_ISREG(st.st_mode)) {
      /* Top-level .rp file -> cache as module name */
      if (!isRpFile(entry->d_name)) continue;
      char *moduleName = moduleNameFromFilename(entry->d_name);
      if (!moduleName) continue;
      cacheAdd(moduleName, filepath, priority);
      free(moduleName);
    } else if (S_ISDIR(st.st_mode)) {
      /* Subdirectory -> check for index.rp */
      char indexpath[2048];
      snprintf(indexpath, sizeof(indexpath), "%s/index.rp", filepath);
      struct stat ist;
      if (stat(indexpath, &ist) == 0 && S_ISREG(ist.st_mode)) {
        /* A package directory wins over a same-named top-level file. */
        cacheAdd(entry->d_name, indexpath, priority + 1);
      }
    }
  }

  closedir(dir);
}

/**
 * Initialize module loader.
 * Called once at startup.
 *
 * Priority (highest first):
 * 1. Project archive: ./modules/rupa_modules.tar.gz -> /tmp/rupa-local/
 * 2. User archive: ~/.rupa/rupa_modules.tar.gz -> /tmp/rupa-global/
 * 3. Embedded system archive -> /tmp/rupa-system/extracted/
 *
 * stdlib is only a development fixture and is never scanned at runtime.
 */
void stdlibLoaderInit(void) {
  stdlibCacheCount = 0;

  /* The embedded archive is the system module baseline. */
  {
    char *systemDir = getSystemExtractDir();
    if (extractEmbeddedArchive(systemDir) == 0) {
      strncpy(extractedSystemDir, systemDir, sizeof(extractedSystemDir) - 1);
      char extracted[1024];
      snprintf(extracted, sizeof(extracted), "%s/extracted", systemDir);
      scanDir(extracted, 1);
    }
  }

  /* Extract global archive */
  char *globalArchive = getGlobalArchive();
  if (globalArchive) {
    char *globalDir = getGlobalExtractDir();
    if (extractArchive(globalArchive, globalDir) == 0) {
      strncpy(extractedGlobalDir, globalDir, sizeof(extractedGlobalDir) - 1);
      scanDir(globalDir, 10);
    }
    free(globalArchive);
  }

  /* Extract local archive (overrides global) */
  char *localArchive = getLocalArchive();
  if (localArchive) {
    char *localDir = getLocalExtractDir();
    if (extractArchive(localArchive, localDir) == 0) {
      strncpy(extractedLocalDir, localDir, sizeof(extractedLocalDir) - 1);
      scanDir(localDir, 20);
    }
    free(localArchive);
  }
}

/**
 * Refresh stdlib cache after adding/removing packages.
 */
void stdlibLoaderRefresh(void) {
  stdlibCacheCount = 0;

  /* Re-extract the embedded system archive. */
  {
    char *systemDir = getSystemExtractDir();
    if (extractEmbeddedArchive(systemDir) == 0) {
      strncpy(extractedSystemDir, systemDir, sizeof(extractedSystemDir) - 1);
      char extracted[1024];
      snprintf(extracted, sizeof(extracted), "%s/extracted", systemDir);
      scanDir(extracted, 1);
    }
  }

  /* Re-extract global archive */
  char *globalArchive = getGlobalArchive();
  if (globalArchive) {
    char *globalDir = getGlobalExtractDir();
    if (extractArchive(globalArchive, globalDir) == 0) {
      strncpy(extractedGlobalDir, globalDir, sizeof(extractedGlobalDir) - 1);
      scanDir(globalDir, 10);
    }
    free(globalArchive);
  }

  /* Re-extract local archive */
  char *localArchive = getLocalArchive();
  if (localArchive) {
    char *localDir = getLocalExtractDir();
    if (extractArchive(localArchive, localDir) == 0) {
      strncpy(extractedLocalDir, localDir, sizeof(extractedLocalDir) - 1);
      scanDir(localDir, 20);
    }
    free(localArchive);
  }
}

/**
 * Find a stdlib module by name.
 */
const char *stdlibFindModule(const char *name) {
  if (!name) return NULL;

  for (int i = 0; i < stdlibCacheCount; i++) {
    if (strcmp(stdlibCache[i].name, name) == 0) {
      return stdlibCache[i].path;
    }
  }
  return NULL;
}

/* Find a package index by its public namespace name. A package directory
 * and its namespace do not have to share the same name, e.g.
 * `stdlib/database/index.rp` exposes `namespace db`. */
const char *stdlibFindNamespace(const char *name) {
  if (!name) return NULL;

  for (int i = 0; i < stdlibCacheCount; i++) {
    const char *path = stdlibCache[i].path;
    size_t len = strlen(path);
    if (len < 10 || strcmp(path + len - 9, "/index.rp") != 0) continue;

    FILE *f = fopen(path, "r");
    if (!f) continue;

    char line[1024];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
      char ns[128];
      if (sscanf(line, " namespace %127[^ {\t\r\n]", ns) == 1 &&
          strcmp(ns, name) == 0) {
        found = true;
        break;
      }
    }
    fclose(f);

    if (found) return path;
  }

  return NULL;
}

/**
 * Get the count of cached stdlib modules.
 */
int stdlibModuleCount(void) {
  return stdlibCacheCount;
}

/**
 * Cleanup extracted stdlib directories.
 */
void stdlibLoaderCleanup(void) {
  if (extractedSystemDir[0] != '\0' && fileExists(extractedSystemDir)) {
    removeDir(extractedSystemDir);
    extractedSystemDir[0] = '\0';
  }
  if (extractedGlobalDir[0] != '\0' && fileExists(extractedGlobalDir)) {
    removeDir(extractedGlobalDir);
    extractedGlobalDir[0] = '\0';
  }
  if (extractedLocalDir[0] != '\0' && fileExists(extractedLocalDir)) {
    removeDir(extractedLocalDir);
    extractedLocalDir[0] = '\0';
  }
}
