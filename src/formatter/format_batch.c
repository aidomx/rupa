#include <rupa.h>

/*
 * Orkestrasi mode batch: menjalankan formatter atas satu/banyak file
 * (fmtRunFile/fmtRunPaths/formatList) dan mode select berdasarkan index
 * declaration (formatSelect).
 */

int fmtRunFile(const char *path, FILE *out) {
  if (!path || !*path || !out) return 1;

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) return 1;

  if (!readfile(path, state->buffer)) {
    fprintf(stderr, "fmt: cannot read '%s'\n", path);
    gcclean();
    return 1;
  }

  FormatterConfig config = fmtLoadConfig();
  if (fmtSourceConfigCompliant(state->buffer->value, (size_t)state->buffer->length, &config)) {
    fwrite(state->buffer->value, 1, (size_t)state->buffer->length, out);
    gcclean();
    return 0;
  }

  Formatter fmt = {0};
  fmt.out = out;
  fmt.indent = 0;
  fmt.needsIndent = false;
  fmt.lastWasNewline = false;
  fmt.config = &config;

  int result = runFormat(state, &fmt);
  gcclean();
  return result;
}

/* Format one file into a malloc'd buffer (caller frees with free()). */
static int fmtFormatToBuffer(const char *path, char **out, size_t *outLen) {
  *out = NULL;
  if (outLen) *outLen = 0;

  char *buf = NULL;
  size_t len = 0;
  FILE *fp = open_memstream(&buf, &len);
  if (!fp) return 1;

  int result = fmtRunFile(path, fp);
  if (fclose(fp) != 0 && result == 0) result = 1;
  if (result != 0) {
    free(buf);
    return result;
  }
  *out = buf;
  if (outLen) *outLen = len;
  return 0;
}

static int fmtRunPaths(const FmtPathList *list, const int *selected, int selectedCount,
                       const char **excludes, int excludeCount) {
  int result = 0;
  int listed = 0;
  int printed = 0;
  int modified = 0;
  int unchanged = 0;
  int failed = 0;
  for (int i = 0; i < list->count; i++) {
    if (fmtPathExcluded(list->items[i], excludes, excludeCount)) continue;
    /* Numbering counts non-excluded entries so indexes match fmtPrintList(). */
    listed++;
    bool use = selectedCount == 0;
    if (!use) {
      for (int j = 0; j < selectedCount; j++) {
        if (selected[j] == listed) {
          use = true;
          break;
        }
      }
    }
    if (!use) continue;

    if (printed++) printf("\n");

    char *source = fmtReadSource(list->items[i]);
    char *formatted = NULL;
    size_t formattedLen = 0;
    /* fmtRunFile owns its GC cleanup; reinitialize it before each file
     * when batch mode invokes it repeatedly. */
    gcinit(100);
    int r = source ? fmtFormatToBuffer(list->items[i], &formatted, &formattedLen) : 1;
    if (r != 0) {
      failed++;
      result = r;
      printf("%-9s %s\n", "FAILED", list->items[i]);
      free(source);
      free(formatted);
      continue;
    }

    bool changed =
        !source || formattedLen != strlen(source) || memcmp(source, formatted, formattedLen) != 0;
    printf("%-9s %s\n", changed ? "MODIFIED" : "UNCHANGED", list->items[i]);
    if (changed)
      modified++;
    else
      unchanged++;
    fwrite(formatted, 1, formattedLen, stdout);
    if (formattedLen == 0 || formatted[formattedLen - 1] != '\n') printf("\n");
    free(source);
    free(formatted);
  }
  if (!printed) {
    fprintf(stderr, "fmt: no selected .rp files\n");
    return 1;
  }
  printf("\n%d file(s): %d modified, %d unchanged", printed, modified, unchanged);
  if (failed) printf(", %d failed", failed);
  printf("\n");
  return result;
}

int formatList(const char *path, bool listOnly) {
  char resolved[4096];
  if (fmtResolveTestPath(path ? path : "tests", resolved, sizeof(resolved)) != 0) return 1;
  FmtPathList list = {0};
  int result = fmtCollectRp(resolved, &list);
  if (result == 0) qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
  if (result == 0) {
    if (listOnly)
      fmtPrintList(&list, NULL, 0);
    else
      result = fmtRunPaths(&list, NULL, 0, NULL, 0);
  }
  fmtPathListFree(&list);
  return result;
}

static int fmtParseCsvInts(const char *value, int **out, int *count) {
  *out = NULL;
  *count = 0;
  if (!value || !*value) return 1;
  char *copy = strdup(value);
  if (!copy) return 1;
  int capacity = 8;
  int *items = malloc((size_t)capacity * sizeof(*items));
  if (!items) {
    free(copy);
    return 1;
  }
  char *save = NULL;
  for (char *tok = strtok_r(copy, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    char *end = NULL;
    long n = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || n <= 0 || n > INT_MAX) {
      free(items);
      free(copy);
      return 1;
    }
    if (*count >= capacity) {
      capacity *= 2;
      int *tmp = realloc(items, (size_t)capacity * sizeof(*items));
      if (!tmp) {
        free(items);
        free(copy);
        return 1;
      }
      items = tmp;
    }
    items[(*count)++] = (int)n;
  }
  free(copy);
  *out = items;
  return *count > 0 ? 0 : 1;
}

int formatSelect(const char *select, const char *path, const char **excludes, int excludeCount) {
  FmtPathList list = {0};
  int *selected = NULL;
  int selectedCount = 0;
  char root[4096];
  int result = fmtResolveTestPath(path && *path ? path : "tests", root, sizeof(root));
  if (result == 0) result = fmtCollectRp(root, &list);
  if (result == 0) result = fmtParseCsvInts(select, &selected, &selectedCount);
  if (result == 0) {
    qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
    /* Valid range follows the post-exclusion numbering shown by --list. */
    int available = 0;
    for (int i = 0; i < list.count; i++) {
      if (!fmtPathExcluded(list.items[i], excludes, excludeCount)) available++;
    }
    for (int i = 0; i < selectedCount; i++) {
      if (selected[i] > available) {
        fprintf(stderr, "fmt: selection %d is out of range (1-%d)\n", selected[i], available);
        result = 1;
        break;
      }
    }
  }
  if (result == 0) result = fmtRunPaths(&list, selected, selectedCount, excludes, excludeCount);
  free(selected);
  fmtPathListFree(&list);
  return result;
}

