#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * Test dispatcher — satu pintu untuk rupa test [...]
 *
 * Perilaku:
 *   rupa test --list                → tampilkan semua tests
 *   rupa test --list ast            → tampilkan tests/ast
 *   rupa test                       → jalankan tests/syntax
 *   rupa test ast                   → jalankan tests/ast
 *   rupa test --path ast            → sama dengan "rupa test ast"
 *   rupa test --select 1,3          → filter dari tests/syntax
 *   rupa test ast --select 1        → filter dari tests/ast
 *   rupa test --path ast --select 1 → sama dengan di atas (urutan bebas)
 *   rupa test exec                  → tests/execution (--test-exec)
 *   rupa test fmt                   → source vs hasil formatter
 *
 * Kategori yang dikenali: syntax, ast, ir, irexec, exec, semantics,
 * repl, fmt (posisi atau lewat --path, boleh berulang — yang terakhir menang).
 * ================================================================ */

static const struct {
  const char *name;
  const char *dir;  /* subfolder di bawah tests/ */
  const char *flag; /* flag binary yang dipakai */
} testCategories[] = {
  {"syntax", "syntax", "syntax"},       {"ast", "ast", "ast"},
  {"ir", "syntax", "ir"},               {"irexec", "syntax", "irexec"},
  {"exec", "execution", "exec"},         {"semantic", "semantics", "semantic"},
  {"stress", "stress", "stress"},         {"repl", "execution", "repl"},
  {"fmt", "formatter", "fmt"},           {"codegen", "codegen", "codegen"},
};

static const char *testFindCategoryDir(const char *name) {
  if (!name) return NULL;
  for (size_t k = 0; k < sizeof(testCategories) / sizeof(testCategories[0]); k++) {
    if (strcmp(testCategories[k].name, name) == 0) return testCategories[k].dir;
  }
  return NULL;
}

static const char *testFindCategoryFlag(const char *name) {
  if (!name) return NULL;
  for (size_t k = 0; k < sizeof(testCategories) / sizeof(testCategories[0]); k++) {
    if (strcmp(testCategories[k].name, name) == 0) return testCategories[k].flag;
  }
  return NULL;
}

static void testUsage(void) {
  showTestHelp();
}

/* Bangun FmtPathList dari folder test + filter kata kunci opsional.
 * Hanya file .rp di dalam folder yang dikumpulkan, lalu diurutkan —
 * penomoran --select mengikuti urutan ini. */
static int testCollectPaths(const char *dir, const char *keyword, FmtPathList *list) {
  char root[4096];
  snprintf(root, sizeof(root), "tests/%s", dir);

  /* Validasi kategori: folder harus ada agar salah ketik langsung terlihat. */
  struct stat st;
  if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "test: test directory not found: %s\n", root);
    return 1;
  }

  if (fmtCollectRp(root, list) != 0) return 1;
  qsort(list->items, (size_t)list->count, sizeof(*list->items), fmtPathCmp);

  /* Filter kata kunci (substring match, mis. "repl" di folder execution). */
  if (keyword && *keyword) {
    int kept = 0;
    for (int i = 0; i < list->count; i++) {
      if (strstr(list->items[i], keyword)) list->items[kept++] = list->items[i];
    }
    for (int i = kept; i < list->count; i++)
      free(list->items[i]);
    list->count = kept;
  }
  return 0;
}

/* Parse "1,3,7" → daftar index 1-based. Pembagi milik pemanggil. */
static int *testParseSelection(const char *value, int *outCount) {
  *outCount = 0;
  if (!value || !*value) return NULL;

  int capacity = 8;
  int *items = malloc((size_t)capacity * sizeof(*items));
  if (!items) return NULL;

  char *copy = strdup(value);
  if (!copy) {
    free(items);
    return NULL;
  }

  char *save = NULL;
  int count = 0;
  for (char *tok = strtok_r(copy, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    char *end = NULL;
    long n = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || n <= 0 || n > INT_MAX) {
      fprintf(stderr, "test: invalid selection '%s'\n", tok);
      free(items);
      free(copy);
      return NULL;
    }
    if (count >= capacity) {
      capacity *= 2;
      int *tmp = realloc(items, (size_t)capacity * sizeof(*items));
      if (!tmp) {
        free(items);
        free(copy);
        return NULL;
      }
      items = tmp;
    }
    items[count++] = (int)n;
  }
  free(copy);
  *outCount = count;
  return items;
}

/* Pilih file dari list berdasarkan index 1-based --select. */
static char **testSelectFiles(const FmtPathList *list, const int *selected, int selectedCount) {
  char **paths = malloc((size_t)selectedCount * sizeof(*paths));
  if (!paths) return NULL;
  for (int i = 0; i < selectedCount; i++) {
    if (selected[i] > list->count) {
      fprintf(stderr, "test: selection %d is out of range (1-%d)\n", selected[i], list->count);
      free(paths);
      return NULL;
    }
    paths[i] = list->items[selected[i] - 1];
  }
  return paths;
}

/* Jalankan satu grup: dispatch ke runner sesuai jenis kategori. */
static int testRunGroup(const char *kind, char **paths, int count) {
  if (strcmp(kind, "fmt") == 0) {
    testFmt((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "syntax") == 0) {
    test((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "ast") == 0) {
    testAst((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "ir") == 0) {
    testIR((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "irexec") == 0) {
    testIRExec((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "exec") == 0 || strcmp(kind, "semantic") == 0 ||
      strcmp(kind, "stress") == 0) {
    testExec((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "repl") == 0) {
    testRepl((const char **)paths, count);
    return 0;
  }
  if (strcmp(kind, "codegen") == 0) {
    testCodegen((const char **)paths, count);
    return 0;
  }
  return 1;
}

/*
 * Parser argumen test (dipakai testDispatch) — satu bentuk per perintah:
 *
 *   rupa test [category] [n,n]
 *   rupa -t  [category] [n,n]
 *   rupa list [category]   (loader meneruskan sebagai -lt [category])
 *
 * Token posisi: numerik/koma → selection, selain itu → kategori.
 * Flag panjang (--list/--path/--select) dan cluster p/s dihapus:
 * -p setara posisi, -s setara token numerik — tidak perlu dua-duanya.
 */
typedef struct {
  bool wantList;
  const char *selectArg; /* "1,2,3" atau NULL */
  const char *category;  /* kategori (default syntax) */
} TestArgs;

/* "1", "1,2,3" dianggap token selection; kategori tidak mungkin numerik. */
static bool testIsSelectionToken(const char *arg) {
  if (!arg || !*arg) return false;
  for (const char *c = arg; *c; c++) {
    if (*c != ',' && (*c < '0' || *c > '9')) return false;
  }
  return true;
}

static bool testArgsParse(const char *args[], int length, TestArgs *a) {
  memset(a, 0, sizeof(*a));

  for (int i = 0; i < length; i++) {
    const char *arg = args[i];

    if (arg[0] == '-') {
      if (strcmp(arg, "-t") == 0) continue; /* penanda mode test */
      if (strcmp(arg, "-l") == 0) {
        a->wantList = true;
        continue;
      }
      /* Cluster pendek hanya dari huruf l/t, mis. -lt. */
      bool ok = arg[1] != '\0';
      for (const char *c = arg + 1; ok && *c; c++)
        if (*c != 'l' && *c != 't') ok = false;
      if (!ok) {
        fprintf(stderr, "test: unknown option '%s'\n", arg);
        testUsage();
        return false;
      }
      if (strchr(arg, 'l')) a->wantList = true;
      continue;
    }

    /* Token posisi: numerik → selection, non-numerik → kategori. */
    if (testIsSelectionToken(arg)) {
      if (a->selectArg) {
        fprintf(stderr, "test: unexpected argument '%s'\n", arg);
        testUsage();
        return false;
      }
      a->selectArg = arg;
    } else if (!a->category) {
      a->category = arg;
    } else {
      fprintf(stderr, "test: unexpected argument '%s'\n", arg);
      testUsage();
      return false;
    }
  }

  return true;
}

int testDispatch(const char *args[], int length) {
  TestArgs a;
  if (!testArgsParse(args, length, &a)) return 1;

  const char *category = a.category;

  const char *keyword = NULL;
  if (category && !testFindCategoryFlag(category)) {
    /* Bukan kategori yang dikenal → perlakukan sebagai kata kunci filter. */
    keyword = category;
    category = NULL;
  }

  const char *dir = category ? testFindCategoryDir(category) : "syntax";

  /* --- Mode list --- */
  if (a.wantList) {
    if (category || keyword) {
      /* -l <kategori>: daftar folder kategori (keyword boleh menfilter). */
      FmtPathList list = {0};
      if (testCollectPaths(dir, keyword, &list) != 0) return 1;
      fmtPrintList(&list, NULL, 0);
      fmtPathListFree(&list);
    } else {
      /* Tanpa kategori: daftar semua file .rp di tests/. */
      FmtPathList list = {0};
      if (fmtCollectRp("tests", &list) != 0) return 1;
      qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
      fmtPrintList(&list, NULL, 0);
      fmtPathListFree(&list);
    }
    return 0;
  }

  /* --- Mode jalankan --- */
  const char *kind = testFindCategoryFlag(category ? category : "syntax");

  FmtPathList list = {0};
  if (testCollectPaths(dir, keyword, &list) != 0) return 1;

  int result = 0;
  if (a.selectArg) {
    int selectedCount = 0;
    int *selected = testParseSelection(a.selectArg, &selectedCount);
    if (!selected || selectedCount == 0) {
      fprintf(stderr, "test: --select requires indexes (e.g. 1,2,3)\n");
      free(selected);
      fmtPathListFree(&list);
      return 1;
    }
    char **paths = testSelectFiles(&list, selected, selectedCount);
    if (!paths) {
      free(selected);
      fmtPathListFree(&list);
      return 1;
    }
    result = testRunGroup(kind, paths, selectedCount);
    free(paths);
    free(selected);
  } else {
    if (list.count == 0) {
      printf("No .rp files found in tests/%s.\n", dir);
      fmtPathListFree(&list);
      return 1;
    }
    result = testRunGroup(kind, list.items, list.count);
  }

  fmtPathListFree(&list);
  return result;
}
