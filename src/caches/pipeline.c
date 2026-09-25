#include <rupa.h>

/* ============================================================
 * pipeline.c — cache level pipeline (AST + IR)
 *
 * Profil: test multi-grup dalam satu proses (test ir irexec) dan
 * REPL me-pipeline ulang file yang sama — lexer + processGenerate +
 * rewrite adalah pekerjaan O(file) yang identik untuk isi yang sama.
 * Cache ini memindahkan hasilnya: kunci = path + mtime + mtime_nsec
 * + size, nilai = (Node*, IRModule*).
 *
 * Bukan cache tracing: GC rupa adalah arena yang dilepas penuh di
 * gcclean() (bootstrap/loader.c), bukan antar file. Karena itu
 * pointer AST/IR yang dicache tetap valid sepanjang proses — pola
 * yang sama dengan name interning di semantic/symbol.c.
 * ============================================================ */

typedef struct PipelineEntry {
  char *path;
  Node *node;
  IRModule *ir;
  time_t mtime;
  long mtime_nsec;
  off_t size;
  bool valid; /* false = invalidation marker (file berubah) */
} PipelineEntry;

static PipelineEntry *g_entries = NULL;
static int g_count = 0;
static int g_capacity = 0;
static int g_hits = 0;
static int g_misses = 0;

static long pipelineMtimeNsec(const struct stat *st) {
#if defined(__APPLE__)
  return st->st_mtimespec.tv_nsec;
#elif defined(_WIN32)
  return 0;
#else
  return st->st_mtim.tv_nsec;
#endif
}

/* Metadata disk valid untuk entry? Sambil validasi, entry basi
 * ditandai invalid (soft invalidation — alokasi tetap di arena GC). */
static bool pipelineEntryFresh(PipelineEntry *e, const struct stat *st) {
  bool fresh = e->mtime == st->st_mtime && e->mtime_nsec == pipelineMtimeNsec(st) &&
               e->size == st->st_size;
  if (!fresh) e->valid = false;
  return fresh;
}

static PipelineEntry *pipelineFind(const char *path, const struct stat *st) {
  for (int i = 0; i < g_count; i++) {
    if (strcmp(g_entries[i].path, path) != 0) continue;
    if (g_entries[i].valid && pipelineEntryFresh(&g_entries[i], st)) return &g_entries[i];
    return NULL; /* marker invalid → selalu miss */
  }
  return NULL;
}

bool pipelineCacheGet(const char *path, Node **outNode, IRModule **outIr) {
  if (outNode) *outNode = NULL;
  if (outIr) *outIr = NULL;
  if (!path || !*path) return false;

  struct stat st;
  if (stat(path, &st) != 0) return false;

  PipelineEntry *e = pipelineFind(path, &st);
  if (!e) {
    g_misses++;
    return false;
  }

  g_hits++;
  if (outNode) *outNode = e->node;
  if (outIr) *outIr = e->ir;
  return true;
}

void pipelineCachePut(const char *path, Node *node, IRModule *ir) {
  if (!path || !*path || !node) return;

  struct stat st;
  if (stat(path, &st) != 0) return;

  /* Path sudah terdaftar (mis. put AST-only lalu upgrade dengan IR)?
   * Refresh pointer + metadata, jangan dobel entri. */
  for (int i = 0; i < g_count; i++) {
    if (strcmp(g_entries[i].path, path) == 0) {
      g_entries[i].node = node;
      g_entries[i].ir = ir;
      g_entries[i].mtime = st.st_mtime;
      g_entries[i].mtime_nsec = pipelineMtimeNsec(&st);
      g_entries[i].size = st.st_size;
      g_entries[i].valid = true;
      return;
    }
  }

  if (g_count == g_capacity) {
    int cap = g_capacity ? g_capacity * 2 : 16;
    PipelineEntry *next = realloc(g_entries, sizeof(PipelineEntry) * (size_t)cap);
    if (!next) return; /* cache gagal tumbuh: jalur tanpa cache tetap benar */
    g_entries = next;
    g_capacity = cap;
  }

  PipelineEntry *e = &g_entries[g_count++];
  memset(e, 0, sizeof(*e));
  e->path = gcdup(path);
  if (!e->path) {
    g_count--;
    return;
  }
  e->node = node;
  e->ir = ir;
  e->mtime = st.st_mtime;
  e->mtime_nsec = pipelineMtimeNsec(&st);
  e->size = st.st_size;
  e->valid = true;
}

void pipelineCacheReset(void) {
  if (g_entries) {
    free(g_entries);
    g_entries = NULL;
  }
  g_count = 0;
  g_capacity = 0;
  g_hits = 0;
  g_misses = 0;
}

int pipelineCacheHits(void) { return g_hits; }

int pipelineCacheMisses(void) { return g_misses; }
