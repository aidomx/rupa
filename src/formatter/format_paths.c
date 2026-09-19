#include <rupa.h>

/*
 * Utilitas path/file untuk mode batch formatter: mengumpulkan file .rp,
 * exclude list, resolusi path uji, dan pembacaan source.
 */

/* ==================== Batch formatter / test selection ==================== */

void fmtPathListFree(FmtPathList *list) {
  if (!list) return;
  for (int i = 0; i < list->count; i++)
    free(list->items[i]);
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

static int fmtPathListAdd(FmtPathList *list, const char *path) {
  if (!list || !path) return 1;
  if (list->count >= list->capacity) {
    int cap = list->capacity ? list->capacity * 2 : 32;
    char **items = realloc(list->items, (size_t)cap * sizeof(*items));
    if (!items) return 1;
    list->items = items;
    list->capacity = cap;
  }
  list->items[list->count] = strdup(path);
  if (!list->items[list->count]) return 1;
  list->count++;
  return 0;
}

int fmtPathCmp(const void *a, const void *b) {
  const char *pa = *(const char *const *)a;
  const char *pb = *(const char *const *)b;
  return strcmp(pa, pb);
}

static bool fmtHasRpExtension(const char *path) {
  const char *dot = strrchr(path, '.');
  return dot && strcmp(dot, ".rp") == 0;
}

int fmtCollectRp(const char *root, FmtPathList *list) {
  struct stat st;
  if (stat(root, &st) != 0) {
    fprintf(stderr, "fmt: path not found: %s\n", root);
    return 1;
  }
  if (S_ISREG(st.st_mode)) {
    return fmtHasRpExtension(root) ? fmtPathListAdd(list, root) : 0;
  }
  if (!S_ISDIR(st.st_mode)) return 0;

  DIR *dir = opendir(root);
  if (!dir) {
    fprintf(stderr, "fmt: cannot open directory '%s'\n", root);
    return 1;
  }

  struct dirent *entry;
  int result = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    size_t len = strlen(root) + strlen(entry->d_name) + 2;
    char *path = malloc(len);
    if (!path) {
      result = 1;
      break;
    }
    snprintf(path, len, "%s/%s", root, entry->d_name);
    result = fmtCollectRp(path, list);
    free(path);
    if (result) break;
  }
  closedir(dir);
  return result;
}

bool fmtPathExcluded(const char *path, const char **excludes, int count) {
  for (int i = 0; i < count; i++) {
    const char *ex = excludes[i];
    if (!ex || !*ex) continue;
    size_t n = strlen(ex);
    if (strncmp(path, ex, n) == 0 &&
        (path[n] == '\0' || path[n] == '/' || (n > 0 && ex[n - 1] == '/')))
      return true;
  }
  return false;
}

int fmtResolveTestPath(const char *path, char *out, size_t outSize) {
  if (!path || !*path || !out || outSize == 0) return 1;
  struct stat st;
  if (stat(path, &st) == 0) {
    snprintf(out, outSize, "%s", path);
    return 0;
  }
  if (strncmp(path, "tests/", 6) == 0) {
    snprintf(out, outSize, "%s", path);
    return 0;
  }
  snprintf(out, outSize, "tests/%s", path);
  return 0;
}

void fmtPrintList(const FmtPathList *list, const char **excludes, int excludeCount) {
  int index = 0;
  for (int i = 0; i < list->count; i++) {
    if (fmtPathExcluded(list->items[i], excludes, excludeCount)) continue;
    printf("%d. %s\n", ++index, list->items[i]);
  }
  if (index == 0) printf("No .rp files found.\n");
}

/* Read a file's raw source text (malloc'd, NUL-terminated) or NULL. */
char *fmtReadSource(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  char *text = NULL;
  if (fseek(fp, 0, SEEK_END) == 0) {
    long size = ftell(fp);
    if (size >= 0) {
      rewind(fp);
      text = malloc((size_t)size + 1);
      if (text) {
        size_t got = fread(text, 1, (size_t)size, fp);
        text[got] = '\0';
      }
    }
  }
  fclose(fp);
  return text;
}

/* Run the formatter over one file into `out`. Owns its GC cleanup;
 * caller must reinitialize GC before each invocation in batch mode. */
