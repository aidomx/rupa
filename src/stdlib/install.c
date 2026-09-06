#include <rupa.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOCAL_ARCHIVE "modules/rupa_modules.tar.gz"

/**
 * Check if a file exists.
 */
static bool pathExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

/**
 * Read a simple string value from module.json.
 * Looks for "key": "value" pattern.
 */
static bool readJsonString(const char *json, const char *key, char *out,
                           size_t outSize) {
  char pattern[256];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);

  const char *pos = strstr(json, pattern);
  if (!pos)
    return false;

  /* Find the colon */
  pos = strchr(pos + strlen(pattern), ':');
  if (!pos)
    return false;
  pos++;

  /* Skip whitespace */
  while (*pos == ' ' || *pos == '\t')
    pos++;

  /* Expect opening quote */
  if (*pos != '"')
    return false;
  pos++;

  /* Read until closing quote */
  size_t i = 0;
  while (*pos && *pos != '"' && i < outSize - 1) {
    out[i++] = *pos++;
  }
  out[i] = '\0';
  return i > 0;
}

/**
 * Read module.json and extract name, version, author, description.
 */
static bool readModuleJson(const char *dir, char *name, size_t nameSize,
                           char *version, size_t versionSize, char *author,
                           size_t authorSize, char *desc, size_t descSize) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/module.json", dir);

  FILE *fp = fopen(path, "r");
  if (!fp)
    return false;

  /* Read entire file */
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size <= 0 || size > 65536) {
    fclose(fp);
    return false;
  }

  char *json = malloc(size + 1);
  if (!json) {
    fclose(fp);
    return false;
  }
  fread(json, 1, size, fp);
  json[size] = '\0';
  fclose(fp);

  bool found = false;
  if (readJsonString(json, "name", name, nameSize))
    found = true;
  if (readJsonString(json, "version", version, versionSize))
    found = true;
  if (readJsonString(json, "author", author, authorSize))
    found = true;
  if (readJsonString(json, "description", desc, descSize))
    found = true;

  free(json);
  return found;
}

/**
 * List packages in a tar.gz archive.
 */
static int listArchive(const char *archivePath, const char *label) {
  if (!pathExists(archivePath)) {
    printf("%s: No archive found.\n", label);
    return 0;
  }

  /* Create temp extraction directory */
  char extractDir[1024];
  snprintf(extractDir, sizeof(extractDir), "/tmp/rupa-pkg-%d", getpid());

  /* Extract archive */
  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
           "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null", extractDir,
           archivePath, extractDir);
  system(cmd);

  /* Scan for packages */
  printf("%s:\n\n", label);
  printf("%-16s %-8s %-12s %s\n", "Package", "Version", "Author",
         "Description");
  printf("%-16s %-8s %-12s %s\n", "-------", "-------", "------",
         "-----------");

  int count = 0;
  DIR *dir = opendir(extractDir);
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.')
        continue;

      char pkgDir[1024];
      snprintf(pkgDir, sizeof(pkgDir), "%s/%s", extractDir, entry->d_name);

      struct stat st;
      if (stat(pkgDir, &st) != 0 || !S_ISDIR(st.st_mode))
        continue;

      /* Read module.json */
      char name[128] = "";
      char version[32] = "";
      char author[128] = "";
      char desc[256] = "";

      if (readModuleJson(pkgDir, name, sizeof(name), version, sizeof(version),
                         author, sizeof(author), desc, sizeof(desc))) {
        /* Use name from module.json if available */
        if (name[0] == '\0')
          strncpy(name, entry->d_name, sizeof(name) - 1);
        if (version[0] == '\0')
          strncpy(version, "1.0.0", sizeof(version) - 1);
      } else {
        /* Fallback: use directory name */
        strncpy(name, entry->d_name, sizeof(name) - 1);
        strncpy(version, "1.0.0", sizeof(version) - 1);
      }

      /* Truncate description for display */
      if (strlen(desc) > 30) {
        desc[27] = '.';
        desc[28] = '.';
        desc[29] = '.';
        desc[30] = '\0';
      }

      printf("%-16s %-8s %-12s %s\n", name, version,
             author[0] ? author : "-", desc[0] ? desc : "-");
      count++;
    }
    closedir(dir);
  }

  if (count == 0) {
    printf("  (none)\n");
  }

  /* Cleanup */
  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", extractDir);
  system(cmd);

  return 0;
}

/**
 * Show usage for module management.
 */
static void showUsage(void) {
  fprintf(stderr, "Usage: rupa <command> [args]\n\n");
  fprintf(stderr, "Package management:\n");
  fprintf(stderr, "  rupa add <package> <path>     Add to local archive\n");
  fprintf(stderr, "  rupa add -g <package> <path>  Add to global archive\n");
  fprintf(stderr, "  rupa add <directory>          Auto-detect name, local\n");
  fprintf(stderr, "  rupa add -g <directory>       Auto-detect name, global\n");
  fprintf(stderr, "  rupa remove <package>         Remove from local archive\n");
  fprintf(stderr,
          "  rupa remove -g <package>      Remove from global archive\n");
  fprintf(stderr, "  rupa list                     List local packages\n");
  fprintf(stderr, "  rupa list -g                  List global packages\n");
  fprintf(stderr, "\nArchives:\n");
  fprintf(stderr, "  Local:  ./modules/rupa_modules.tar.gz\n");
  fprintf(stderr, "  Global: ~/.rupa/rupa_modules.tar.gz\n");
}

/**
 * Check if curl or wget is available.
 */
static const char *findDownloader(void) {
  if (system("command -v curl >/dev/null 2>&1") == 0)
    return "curl";
  if (system("command -v wget >/dev/null 2>&1") == 0)
    return "wget";
  return NULL;
}

/**
 * Download file using curl or wget.
 */
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

/**
 * Add a local package to stdlib archive.
 */
static int addLocalPackage(const char *packageName, const char *sourcePath,
                           bool global, const char *version) {
  /* Determine archive path */
  const char *archivePath = global ? "~/.rupa/rupa_modules.tar.gz" : LOCAL_ARCHIVE;

  /* Create temp directory */
  char tempDir[1024];
  snprintf(tempDir, sizeof(tempDir), "/tmp/rupa-add-%d", getpid());
  char cmd[2048];

  /* If archive exists, extract it first */
  if (pathExists(archivePath)) {
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null",
             tempDir, archivePath, tempDir);
    system(cmd);
  } else {
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", tempDir);
    system(cmd);
  }

  /* Copy source to temp dir */
  char pkgDir[1024];
  snprintf(pkgDir, sizeof(pkgDir), "%s/%s", tempDir, packageName);

  /* Remove existing package if any */
  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pkgDir);
  system(cmd);

  /* Copy source directory */
  snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\"", sourcePath, pkgDir);
  if (system(cmd) != 0) {
    fprintf(stderr, "Error: Failed to copy package from %s\n", sourcePath);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return -1;
  }

  /* Create .version file */
  char versionFile[1024];
  snprintf(versionFile, sizeof(versionFile), "%s/.version", pkgDir);
  FILE *fp = fopen(versionFile, "w");
  if (fp) {
    fprintf(fp, "%s", version);
    fclose(fp);
  }

  /* Ensure archive directory exists */
  const char *lastSlash = strrchr(archivePath, '/');
  if (lastSlash) {
    char archiveDir[1024];
    size_t dirLen = (size_t)(lastSlash - archivePath);
    strncpy(archiveDir, archivePath, dirLen);
    archiveDir[dirLen] = '\0';
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", archiveDir);
    system(cmd);
  }

  /* Create archive */
  snprintf(cmd, sizeof(cmd), "tar czf \"%s\" -C \"%s\" .", archivePath, tempDir);
  if (system(cmd) != 0) {
    fprintf(stderr, "Error: Failed to create archive\n");
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return -1;
  }

  /* Cleanup */
  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
  system(cmd);

  printf("Package '%s' (v%s) added to %s.\n", packageName, version,
         global ? "global" : "local");

  /* Refresh stdlib cache */
  stdlibLoaderRefresh();

  return 0;
}

/**
 * Remove a package from stdlib archive.
 */
static int removeLocalPackage(const char *packageName, bool global) {
  char *globalPath = global ? getGlobalArchive() : NULL;
  const char *archivePath = global ? globalPath : LOCAL_ARCHIVE;

  if (!pathExists(archivePath)) {
    fprintf(stderr, "No archive found.\n");
    return 1;
  }

  /* Extract archive */
  char tempDir[1024];
  snprintf(tempDir, sizeof(tempDir), "/tmp/rupa-rm-%d", getpid());
  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null",
           tempDir, archivePath, tempDir);
  system(cmd);

  /* Check if package exists */
  char pkgDir[1024];
  snprintf(pkgDir, sizeof(pkgDir), "%s/%s", tempDir, packageName);
  struct stat st;
  if (stat(pkgDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "Package '%s' not found.\n", packageName);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
    system(cmd);
    return 1;
  }

  /* Remove package */
  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pkgDir);
  system(cmd);

  /* Recreate archive */
  snprintf(cmd, sizeof(cmd), "tar czf \"%s\" -C \"%s\" .", archivePath, tempDir);
  system(cmd);

  /* Cleanup */
  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tempDir);
  system(cmd);

  printf("Package '%s' removed.\n", packageName);

  /* Refresh stdlib cache */
  stdlibLoaderRefresh();

  return 0;
}

/**
 * Main entry point for module management.
 */
int stdlibManage(const char *args[], int length) {
  if (length < 1) {
    showUsage();
    return 1;
  }

  const char *command = args[0];

  /* Check for -g flag at top level */
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

    /* Check if source is a directory with index.rp */
    struct stat st;
    if (stat(sourcePath, &st) == 0 && S_ISDIR(st.st_mode)) {
      /* Auto-detect: use directory name as package name */
      const char *lastSlash = strrchr(sourcePath, '/');
      packageName = lastSlash ? lastSlash + 1 : sourcePath;
    } else {
      /* Explicit: rupa add <package> <path> */
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
      listArchive(LOCAL_ARCHIVE, "Local packages (./modules/rupa_modules.tar.gz)");
    }
    return 0;
  }

  fprintf(stderr, "Unknown command: %s\n", command);
  showUsage();
  return 1;
}
