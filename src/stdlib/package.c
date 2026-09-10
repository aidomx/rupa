#include <rupa.h>

/* Package utilities — extracted from install.c for modularity.
 * Contains path checking, JSON reading, and archive listing.
 * CLI management remains in install.c. */

bool pathExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

bool readJsonString(const char *json, const char *key, char *out,
                    size_t outSize) {
  char pattern[256];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);

  const char *pos = strstr(json, pattern);
  if (!pos)
    return false;

  pos = strchr(pos + strlen(pattern), ':');
  if (!pos)
    return false;
  pos++;

  while (*pos == ' ' || *pos == '\t')
    pos++;

  if (*pos != '"')
    return false;
  pos++;

  size_t i = 0;
  while (*pos && *pos != '"' && i < outSize - 1) {
    out[i++] = *pos++;
  }
  out[i] = '\0';
  return i > 0;
}

bool readModuleJson(const char *dir, char *name, size_t nameSize,
                    char *version, size_t versionSize,
                    char *author, size_t authorSize,
                    char *description, size_t descSize) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/module.json", dir);

  FILE *f = fopen(path, "r");
  if (!f)
    return false;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *json = malloc(size + 1);
  if (!json) {
    fclose(f);
    return false;
  }
  fread(json, 1, size, f);
  json[size] = '\0';
  fclose(f);

  bool ok = true;
  if (name)
    ok = ok && readJsonString(json, "name", name, nameSize);
  if (version)
    ok = ok && readJsonString(json, "version", version, versionSize);
  if (author)
    ok = ok && readJsonString(json, "author", author, authorSize);
  if (description)
    ok = ok && readJsonString(json, "description", description, descSize);

  free(json);
  return ok;
}

int listArchive(const char *archivePath, const char *label) {
  if (!pathExists(archivePath)) {
    printf("%s: No archive found.\n", label);
    return 0;
  }

  char extractDir[1024];
  snprintf(extractDir, sizeof(extractDir), "/tmp/rupa-pkg-%d", getpid());

  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
           "mkdir -p \"%s\" && tar xzf \"%s\" -C \"%s\" 2>/dev/null",
           extractDir, archivePath, extractDir);
  system(cmd);

  printf("%s:\n", label);
  snprintf(cmd, sizeof(cmd), "ls -1 \"%s\"", extractDir);
  int ret = system(cmd);

  snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", extractDir);
  system(cmd);

  return ret;
}
