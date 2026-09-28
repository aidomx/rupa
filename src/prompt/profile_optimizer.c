#include <rupa.h>
#include "profile.h"
#include "test_internal.h"

/* ================================================================
 * profile_optimizer.c — mode `rupa profile --optimizer`
 *
 * "Cek file-file yang melakukan pekerjaan duplikasi dan lakukan
 * patch perbaikan" (todos item 3).
 *
 * Mesin deteksinya sudah ada di compiler: grammarParseExpr (grammar_
 * expression.c) mem-publish ekspresi terluar tiap file ke pool kanonik
 * (caches/syntax/canonical.c); file berikutnya yang menemukan stream
 * token identik meng-ADOPT subtree itu tanpa parse ulang. Optimizer
 * mengaktifkan trace event pool, memindai file .rp satu per satu,
 * lalu melaporkan grup ekspresi yang di-adopt (>= 2 kemunculan) —
 * lengkap dengan file/baris pertama & terakhir dan SARAAN patch
 * (ekstraksi helper).
 *
 * Mode ini TIDAK mengubah file: patch otomatis terhadap sumber user
 * berisiko merusak kode, jadi "patch perbaikan" berupa saran eksplisit
 * yang bisa dieksekusi user.
 *
 * Keterbatasan yang disengaja: pool hanya berisi EKSPRESI MURNI
 * (allowlist tipe parser) dan hanya ekspresi terluar (depth 0) —
 * deteksi melaporkan apa yang dilihat parser, bukan audit lengkap.
 * ================================================================ */

#define OPT_MAX_FILES 256
#define OPT_MAX_EVENTS 256
#define OPT_MAX_TEXT 128

typedef struct {
  char *text;      /* teks kasar ekspresi (malloc) */
  int count;       /* total kemunculan (publish + adopt) */
  char *firstFile; /* kemunculan pertama (malloc) */
  int firstLine;
  char *lastFile; /* kemunculan terakhir (malloc) */
  int lastLine;
} OptGroup;

typedef struct {
  OptGroup *items;
  int count;
  int capacity;
} OptGroups;

/* ------------------------------------------------------------------
 * Helper teks (malloc biasa — hidup sampai laporan selesai dicetak)
 * ------------------------------------------------------------------ */

static char *optStrdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *out = malloc(n);
  if (out)
    memcpy(out, s, n);
  return out;
}

/* Teks sumber baris `line` (1-based), tanpa newline. malloc'd. */
static char *optLineSource(const char *src, int line) {
  if (!src)
    return optStrdup("");
  int current = 1;
  const char *p = src;
  while (*p && current < line) {
    if (*p == '\n')
      current++;
    p++;
  }
  const char *end = strchr(p, '\n');
  size_t len = end ? (size_t)(end - p) : strlen(p);
  char *out = malloc(len + 1);
  if (!out)
    return optStrdup("");
  memcpy(out, p, len);
  out[len] = '\0';
  return out;
}

/* ------------------------------------------------------------------
 * Grup duplikasi
 * ------------------------------------------------------------------ */

/* Index entri pool tidak disimpan per grup — identitas grup cukup dari
 * teks kasarnya. Array paralel memetakan entry → grup saat mengumpulkan. */
static int g_groupEntry[OPT_MAX_FILES]; /* index entri pool per grup */

static OptGroup *optGroupFor(OptGroups *g, int entry) {
  for (int i = 0; i < g->count; i++)
    if (g_groupEntry[i] == entry)
      return &g->items[i];
  return NULL;
}

static OptGroup *optGroupNew(OptGroups *g, int entry, const char *text) {
  if (g->count >= OPT_MAX_FILES)
    return NULL; /* batas laporan — terima saja */
  if (g->count == g->capacity) {
    int cap = g->capacity ? g->capacity * 2 : 8;
    OptGroup *next = realloc(g->items, sizeof(OptGroup) * (size_t)cap);
    if (!next)
      return NULL;
    g->items = next;
    g->capacity = cap;
  }
  OptGroup *item = &g->items[g->count];
  memset(item, 0, sizeof(*item));
  item->text = optStrdup(text);
  g_groupEntry[g->count] = entry;
  g->count++;
  return item;
}

static void optGroupsFree(OptGroups *g) {
  for (int i = 0; i < g->count; i++) {
    free(g->items[i].text);
    free(g->items[i].firstFile);
    free(g->items[i].lastFile);
  }
  free(g->items);
  g->items = NULL;
  g->count = 0;
  g->capacity = 0;
}

static int optGroupCmp(const void *a, const void *b) {
  const OptGroup *ga = a, *gb = b;
  return gb->count - ga->count; /* terbanyak dulu */
}

/* ------------------------------------------------------------------
 * Rekursi direktori (pola scandir sederhana)
 * ------------------------------------------------------------------ */

static bool optHasRpSuffix(const char *name) {
  size_t n = strlen(name);
  return n > 3 && strcmp(name + n - 3, ".rp") == 0;
}

static bool optSkipEntry(const char *name) {
  return strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
         strcmp(name, "modules") == 0 || strcmp(name, "stdlib") == 0 ||
         strcmp(name, "node_modules") == 0 || name[0] == '.';
}

/* Kumpulkan path .rp di bawah root (rekursif). files = array malloc'd
 * string (diurutkan pemanggil). Return jumlah yang ditambahkan. */
static int optCollectFiles(const char *root, char ***files, int *count, int *capacity,
                           int max) {
  DIR *dir = opendir(root);
  if (!dir)
    return 0;

  struct dirent *entry;
  int added = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (optSkipEntry(entry->d_name))
      continue;

    char *path = modJoinPath(root, entry->d_name);
    if (!path)
      continue;

    struct stat st;
    if (stat(path, &st) != 0) {
      free(path);
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      added += optCollectFiles(path, files, count, capacity, max);
      free(path);
    } else if (S_ISREG(st.st_mode) && optHasRpSuffix(entry->d_name)) {
      if (*count >= max) {
        free(path);
        break;
      }
      if (*count == *capacity) {
        int cap = *capacity ? *capacity * 2 : 16;
        char **next = realloc(*files, sizeof(char *) * (size_t)cap);
        if (!next) {
          free(path);
          break;
        }
        *files = next;
        *capacity = cap;
      }
      (*files)[(*count)++] = optStrdup(path);
      added++;
      free(path);
    } else {
      free(path);
    }
  }

  closedir(dir);
  return added;
}

static int optPathCmp(const void *a, const void *b) {
  const char *pa = *(const char *const *)a;
  const char *pb = *(const char *const *)b;
  return strcmp(pa, pb);
}

/* ------------------------------------------------------------------
 * Inti optimizer
 * ------------------------------------------------------------------ */

int profileOptimizer(const char *root) {
  /* Root default: direktori kerja saat ini. */
  static char cwdBuf[MAX_PATH_LENGTH];
  if (!root || !*root) {
    if (!getcwd(cwdBuf, sizeof(cwdBuf))) {
      fprintf(stderr, "rupa profile --optimizer: tidak bisa membaca cwd\n");
      return 1;
    }
    root = cwdBuf;
  }

  /* Kumpulkan file .rp (diurutkan agar laporan deterministik). */
  char **files = NULL;
  int fileCount = 0;
  int fileCapacity = 0;
  optCollectFiles(root, &files, &fileCount, &fileCapacity, OPT_MAX_FILES);
  if (fileCount > 1)
    qsort(files, (size_t)fileCount, sizeof(char *), optPathCmp);

  OptGroups groups = {0};
  int parseFail = 0;
  SyntaxCanonEvent events[OPT_MAX_EVENTS];

  syntaxCanonicalReset();
  syntaxCanonicalTraceEnable(true);

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "rupa profile --optimizer: gagal membuat state\n");
    syntaxCanonicalTraceEnable(false);
    return 1;
  }

  for (int fi = 0; fi < fileCount; fi++) {
    const char *path = files[fi];
    Buffer *buffer = NULL;
    Token *tokens = NULL;
    Node *node = NULL;

    if (!testLexParse(state, path, &buffer, &tokens, &node)) {
      parseFail++;
      continue;
    }

    /* Ambil event publish/adopt file ini, kumpulkan ke grup. Event
     * didedupe per (entry, line): satu baris sumber = satu kemunculan
     * (publish + adopt beruntun & adopt lintas-file untuk teks sama
     * tidak boleh menghitung dobel). */
    int n = syntaxCanonicalTakeEvents(events, OPT_MAX_EVENTS);
    int seenEntry[OPT_MAX_EVENTS];
    int seenLine[OPT_MAX_EVENTS];
    int seenCount = 0;
    for (int k = 0; k < n; k++) {
      const SyntaxCanonEvent *ev = &events[k];
      if (ev->kind != CANON_EVENT_ADOPT && ev->kind != CANON_EVENT_PUBLISH)
        continue;

      bool dup = false;
      for (int s = 0; s < seenCount; s++)
        if (seenEntry[s] == ev->entry && seenLine[s] == ev->line) {
          dup = true;
          break;
        }
      if (dup)
        continue;
      if (seenCount < OPT_MAX_EVENTS) {
        seenEntry[seenCount] = ev->entry;
        seenLine[seenCount] = ev->line;
        seenCount++;
      }

      char text[OPT_MAX_TEXT];
      if (syntaxCanonicalEntryText(ev->entry, text, sizeof(text)) < 0)
        continue;

      OptGroup *g = optGroupFor(&groups, ev->entry);
      if (!g) {
        g = optGroupNew(&groups, ev->entry, text);
        if (!g)
          continue;
      }

      g->count++;
      free(g->lastFile);
      g->lastFile = optStrdup(path);
      g->lastLine = ev->line;

      if (!g->firstFile) {
        g->firstFile = optStrdup(path);
        g->firstLine = ev->line;
      }
    }
  }

  syntaxCanonicalTraceEnable(false);
  syntaxCanonicalReset();

  /* ---- Laporan ---- */
  int dupGroups = 0, dupOccurrences = 0;
  for (int i = 0; i < groups.count; i++)
    if (groups.items[i].count >= 2) {
      dupGroups++;
      dupOccurrences += groups.items[i].count;
    }

  printf("Rupa Optimizer Report v1.0\n\n");
  printf("Scan      : %s (%d file .rp)\n", root, fileCount);
  if (parseFail > 0)
    printf("Gagal parse: %d file (dilewati)\n", parseFail);
  printf("Duplikasi : %d grup ekspresi, %d kemunculan\n\n", dupGroups,
         dupOccurrences);

  if (dupGroups == 0) {
    printf("Tidak ada duplikasi ekspresi lintas file yang terdeteksi.\n");
  } else {
    /* Urutkan salinan grup (index stabil tak dibutuhkan lagi). */
    qsort(groups.items, (size_t)groups.count, sizeof(OptGroup), optGroupCmp);

    int rank = 0;
    for (int i = 0; i < groups.count && rank < 20; i++) {
      OptGroup *g = &groups.items[i];
      if (g->count < 2)
        continue;
      rank++;

      printf("[%d] x%d  ekspresi: %s\n", rank, g->count,
             g->text ? g->text : "(?)");
      printf("    pertama : %s:%d\n", g->firstFile ? g->firstFile : "-", g->firstLine);
      printf("    terakhir: %s:%d\n", g->lastFile ? g->lastFile : "-", g->lastLine);

      /* Saran patch: contoh sumber baris pertama + resep ekstraksi. */
      if (g->firstFile) {
        State *peek = createGlobalState(1, false);
        if (peek && peek->buffer && readfile(g->firstFile, peek->buffer)) {
          char *src = optLineSource(peek->buffer->value, g->firstLine);
          printf("    sumber  : %s\n", src);
          free(src);
        }
        /* peek dibiarkan hidup di GC arena — dilepas gcclean(). */
      }
      printf("    saran   : ekstraksi jadi helper/fungsi bersama lalu panggil\n");
      printf("              dari kedua file — pool kanonik parser hanya\n");
      printf("              berbagi hasil parse, pekerjaan duplikat tetap\n");
      printf("              ada di sumber.\n\n");
    }
    if (dupGroups > rank)
      printf("(+ %d grup lain di bawah ambang tampilan)\n", dupGroups - rank);
  }

  /* ---- Bersih-bersih ---- */
  optGroupsFree(&groups);
  for (int i = 0; i < fileCount; i++)
    free(files[i]);
  free(files);

  return 0;
}
