#include <rupa.h>

/* ============================================================
 * expr.c — memo parse per-produksi (level ekspresi)
 *
 * Choke point-nya grammarParseExpr (semua ekspresi lewat sini).
 * Kunci = (a, b) + FNV-1a atas token ternormalisasi (whitespace
 * dilewati); hit DIVALIDASI dengan perbandingan token per-token —
 * hash hanya penyaring cepat, kebenaran dari verifikasi penuh
 * (aman terhadap kolisi dan pattern `a< b` vs `a <b`).
 *
 * Kenapa per-Request, bukan global: node id AST spesifik pool
 * (Request.node). Memo lintas file butuh kepemilikan AST ulang;
 * di dalam satu file, dua sub-ekspresi identik berbagi node —
 * valid karena semua konsumen membaca node, tidak menulisnya.
 * ============================================================ */

#define SYNTAX_MEMO_INIT_CAP 32
#define SYNTAX_MEMO_MAX 4096 /* batas entri per Request */

typedef struct SyntaxMemoEntry {
  int a;             /* span awal (setelah trim whitespace) */
  int b;             /* span akhir (eksklusif) */
  unsigned long h;   /* FNV-1a konten span — penyaring cepat */
  int nodeId;        /* node id hasil parse (pool Request) */
  bool valid;
} SyntaxMemoEntry;

struct SyntaxMemo {
  SyntaxMemoEntry *entries;
  int count;
  int capacity;
};

static int g_hits = 0;
static int g_misses = 0;

static unsigned long syntaxSpanHash(const Token *t, int a, int b) {
  unsigned long x = 1469598103934665603ULL;
  for (int i = a; i < b; i++) {
    const DataToken *d = &t->data[i];
    if (d->type == NEWLINE || d->type == TAB) continue;
    x ^= (unsigned char)d->type;
    x *= 1099511628211ULL;
    if (d->value) {
      for (const char *s = d->value; *s; s++) {
        x ^= (unsigned char)*s;
        x *= 1099511628211ULL;
      }
    }
    x ^= 0xFF;
    x *= 1099511628211ULL;
  }
  return x;
}

/* Verifikasi konten span benar-benar identik (hash bisa kolisi).
 * Whitespace diabaikan di kedua sisi. */
static bool syntaxSpanEqual(const Token *t, int a1, int b1, int a2, int b2) {
  int i = a1, j = a2;
  for (;;) {
    while (i < b1 && (t->data[i].type == NEWLINE || t->data[i].type == TAB)) i++;
    while (j < b2 && (t->data[j].type == NEWLINE || t->data[j].type == TAB)) j++;
    bool e1 = i >= b1, e2 = j >= b2;
    if (e1 || e2) return e1 && e2;
    const DataToken *x = &t->data[i], *y = &t->data[j];
    if (x->type != y->type) return false;
    if (x->value && y->value) {
      if (strcmp(x->value, y->value) != 0) return false;
    } else if (x->value || y->value) {
      return false;
    }
    i++; j++;
  }
}

bool syntaxMemoLookup(struct Request *r, int a, int b, int *outNodeId) {
  struct SyntaxMemo *m = (struct SyntaxMemo *)r->syntaxMemo;
  if (!m || !r->tokens) return false;
  while (a < b && grammarIsWhitespace(r->tokens, a)) a++;
  while (b > a && grammarIsWhitespace(r->tokens, b - 1)) b--;
  if (a >= b) return false;

  unsigned long h = syntaxSpanHash(r->tokens, a, b);
  for (int i = 0; i < m->count; i++) {
    SyntaxMemoEntry *e = &m->entries[i];
    if (e->valid && e->h == h && e->a == a && e->b == b &&
        syntaxSpanEqual(r->tokens, a, b, e->a, e->b)) {
      g_hits++;
      if (outNodeId) *outNodeId = e->nodeId;
      return true;
    }
  }
  g_misses++;
  return false;
}

void syntaxMemoStore(struct Request *r, int a, int b, int nodeId) {
  if (nodeId < 0 || !r->tokens) return;
  struct SyntaxMemo *m = (struct SyntaxMemo *)r->syntaxMemo;
  if (!m) {
    m = gcmall(sizeof(*m));
    if (!m) return;
    m->entries = NULL;
    m->count = 0;
    m->capacity = 0;
    r->syntaxMemo = m;
  }
  if (m->count >= SYNTAX_MEMO_MAX) return;
  while (a < b && grammarIsWhitespace(r->tokens, a)) a++;
  while (b > a && grammarIsWhitespace(r->tokens, b - 1)) b--;
  if (a >= b) return;
  if (m->count == m->capacity) {
    int cap = m->capacity ? m->capacity * 2 : SYNTAX_MEMO_INIT_CAP;
    SyntaxMemoEntry *next = gcrealloc(m->entries, sizeof(SyntaxMemoEntry) * (size_t)cap);
    if (!next) return;
    m->entries = next;
    m->capacity = cap;
  }
  SyntaxMemoEntry *e = &m->entries[m->count++];
  e->a = a;
  e->b = b;
  e->h = syntaxSpanHash(r->tokens, a, b);
  e->nodeId = nodeId;
  e->valid = true;
}

void syntaxMemoDetach(struct Request *r) {
  if (!r || !r->syntaxMemo) return;
  struct SyntaxMemo *m = (struct SyntaxMemo *)r->syntaxMemo;
  gcfree(m->entries);
  gcfree(m);
  r->syntaxMemo = NULL;
}

int syntaxMemoHits(void) { return g_hits; }

int syntaxMemoMisses(void) { return g_misses; }
