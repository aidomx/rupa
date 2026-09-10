#include <rupa.h>

/* Package CLI management — uses package utilities from package.c.
 * Contains download, add, remove, and list operations. */

/* External declarations from package.c */
extern bool pathExists(const char *path);
extern bool readJsonString(const char *json, const char *key, char *out,
                           size_t outSize);
extern bool readModuleJson(const char *dir, char *name, size_t nameSize,
                           char *version, size_t versionSize,
                           char *author, size_t authorSize,
                           char *description, size_t descSize);
extern int listArchive(const char *archivePath, const char *label);

static void showUsage(void) {
  fprintf(stderr, "Usage: rupa <command> [args]\n\n");
  fprintf(stderr, "Package management:\n");
  fprintf(stderr, "  rupa add <package> <path>     Add to local archive\n");
  fprintf(stderr, "  rupa add -g <package> <path>  Add to global archive\n");
  fprintf(stderr, "  rupa add <directory>          Auto-detect name, local\n");
  fprintf(stderr, "  rupa add -g <directory>       Auto-detect name, global\n");
  fprintf(stderr,
          "  rupa remove <package>         Remove from local archive\n");
  fprintf(stderr,
          "  rupa remove -g <package>      Remove from global archive\n");
  fprintf(stderr, "  rupa list                     List local packages\n");
  fprintf(stderr, "  rupa list -g                  List global packages\n");
  fprintf(stderr, "\nArchives:\n");
  fprintf(stderr, "  Local:  ./modules/rupa_modules.tar.gz\n");
  fprintf(stderr, "  Global: ~/.rupa/rupa_modules.tar.gz\n");
}

static const char *findDownloader(void) {
  if (system("command -v curl >/dev/null 2>&1") == 0)
    return "curl";
  if (system("command -v wget >/dev/null 2>&1") == 0)
    return "wget";
  return NULL;
}

static int downloadFile(const char *url, const char *dest) {
  const char *downloader = findDownloader();
  if (!downloader) {
    fprintf(stderr, "Error: curl or wget is required.\n");
    return -1;
  }

  char cmd[2048];
  if (strcmp(downloader, "curl") == 0) {
    snprintf(cmd, sizeof(cmd), "curl -fsSL -o \"%s\" \"%s\"", dest, url);
  } else {
    snprintf(cmd, sizeof(cmd), "wget -q -O \"%s\" \"%s\"", dest, url);
  }

  return system(cmd);
}

static int addLocalPackage(const char *packageName, const char *sourcePath,
                           bool global, const char *version) {
  const char *archivePath =
      global ? "~/.rupa/rupa_modules.tar.gz" : LOCAL_ARCHIVE;

  char tempDir[1024];
  snprintf(tempDir, sizeof(tempDir), "/tmp/rupa-add-%d", getpid());
  char cmd[2048];

  if (pathExists(archivePath)) {
    snprintf(cmd, sizeof(cmd),
             "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null", tempDir,
             archivePath, tempDir);
    system(cmd);
  } else {
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", tempDir);
    system(cmd);
  }

  char pkgDir[1024];
  snprintf(pkgDir, sizeof(pkgDir), "%s/%s", tempDir, packageName);

  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pkgDir);
  system(cmd);

  snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", sourcePath, pkgDir);
  if (system(cmd) != 0) {
    fprintf(stderr, "Error: Failed to copy package from %s\n", sourcePath);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return -1;
  }

  char versionFile[1024];
  snprintf(versionFile, sizeof(versionFile), "%s/.version", pkgDir);
  FILE *fp = fopen(versionFile, "w");
  if (fp) {
    fprintf(fp, "%s", version);
    fclose(fp);
  }

  const char *lastSlash = strrchr(archivePath, '/');
  if (lastSlash) {
    char archiveDir[1024];
    size_t dirLen = (size_t)(lastSlash - archivePath);
    strncpy(archiveDir, archivePath, dirLen);
    archiveDir[dirLen] = '\0';
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", archiveDir);
    system(cmd);
  }

  snprintf(cmd, sizeof(cmd), "tar czf \"%s\" -C \"%s\" .", archivePath,
           tempDir);
  if (system(cmd) != 0) {
    fprintf(stderr, "Error: Failed to create archive\n");
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return -1;
  }

  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
  system(cmd);

  printf("Package '%s' (v%s) added to %s.\n", packageName, version,
         global ? "global" : "local");

  stdlibLoaderRefresh();

  return 0;
}

static int removeLocalPackage(const char *packageName, bool global) {
  char *globalPath = global ? getGlobalArchive() : NULL;
  const char *archivePath = global ? globalPath : LOCAL_ARCHIVE;

  if (!pathExists(archivePath)) {
    fprintf(stderr, "No archive found.\n");
    return 1;
  }

  char tempDir[1024];
  snprintf(tempDir, sizeof(tempDir), "/tmp/rupa-rm-%d", getpid());
  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
           "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null", tempDir,
           archivePath, tempDir);
  system(cmd);

  char pkgDir[1024];
  snprintf(pkgDir, sizeof(pkgDir), "%s/%s", tempDir, packageName);
  struct stat st;
  if (stat(pkgDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "Package '%s' not found.\n", packageName);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return 1;
  }

  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pkgDir);
  system(cmd);

  snprintf(cmd, sizeof(cmd), "tar czf \"%s\" -C \"%s\" .", archivePath,
           tempDir);
  system(cmd);

  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
  system(cmd);

  printf("Package '%s' removed.\n", packageName);

  stdlibLoaderRefresh();

  return 0;
}

int stdlibManage(const char *args[], int length) {
  if (length < 1) {
    showUsage();
    return 1;
  }

  const char *command = args[0];

  bool global = false;
  {
    int idx = 1;
    while (idx < length && args[idx][0] == '-') {
      if (strcmp(args[idx], "-g") == 0) {
        global = true;
        idx++;
      } else {
        break;
      }
    }
  }

  if (strcmp(command, "add") == 0) {
    int argStart = 1;
    if (length > 1 && strcmp(args[1], "-g") == 0) {
      argStart = 2;
    }

    if (length - argStart < 1) {
      fprintf(stderr, "Usage: rupa add [-g] <package> <path> [version]\n");
      fprintf(stderr, "       rupa add [-g] <directory>\n");
      return 1;
    }

    const char *sourcePath = args[argStart];
    const char *packageName = NULL;
    const char *version = "1.0.0";

    struct stat st;
    if (stat(sourcePath, &st) == 0 && S_ISDIR(st.st_mode)) {
      const char *lastSlash = strrchr(sourcePath, '/');
      packageName = lastSlash ? lastSlash + 1 : sourcePath;
    } else {
      if (length - argStart < 2) {
        fprintf(stderr, "Usage: rupa add <package> <path> [version]\n");
        return 1;
      }
      packageName = args[argStart];
      sourcePath = args[argStart + 1];
      if (length - argStart > 2)
        version = args[argStart + 2];
    }

    return addLocalPackage(packageName, sourcePath, global, version);
  }

  if (strcmp(command, "remove") == 0) {
    int argStart = 1;
    if (length > 1 && strcmp(args[1], "-g") == 0) {
      argStart = 2;
    }

    if (length - argStart < 1) {
      fprintf(stderr, "Usage: rupa remove [-g] <package>\n");
      return 1;
    }

    return removeLocalPackage(args[argStart], global);
  }

  if (strcmp(command, "list") == 0) {
    if (global) {
      char *archivePath = getGlobalArchive();
      listArchive(archivePath, "Global packages (~/.rupa/rupa_modules.tar.gz)");
      free(archivePath);
    } else {
      listArchive(LOCAL_ARCHIVE,
                  "Local packages (./modules/rupa_modules.tar.gz)");
    }
    return 0;
  }

  fprintf(stderr, "Unknown command: %s\n", command);
  showUsage();
  return 1;
}
