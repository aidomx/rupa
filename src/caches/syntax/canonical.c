#include <rupa.h>

/* ============================================================
 * canonical.c — pool subtree AST kanonik lintas Request
 *
 * Melengkapi memo per-Request (expr.c): subtree EKSPRESI MURNI
 * dari satu file dipublikasikan ke pool global; file lain yang
 * menemukan span ber-konten token identik meng-ADOPT subtree itu
 * — deep copy + remap id ke pool requestor sendiri (pola
 * parse_atom.c parseInterpExpr), bukan mem-parse ulang.
 *
 * Keamanan kepemilikan (node id = indeks pool!):
 *  - Yang dibagi antar file hanya DATA subtree; id file asal tidak
 *    pernah dipakai file lain — adopt selalu membuat id baru.
 *  - Capture = DFS reachable-set dari root (TIDAK berasumsi id
 *    subtree kontigu — desugar/re-parse membuat node lompatan).
 *  - Allowlist tipe: ekspresi murni tanpa referensi statement/
 *    scope. NODE_OBJECT disengaja di-exclude (entry key = node id
 *    semantiknya belum terisolasi).
 *  - Verifikasi adopt = perbandingan STREAM TOKEN ternormalisasi
 *    yang tersimpan penuh di entri (hash = penyaring cepat saja).
 *    Salah identitas = AST salah = bug kebenaran: tidak dikompromi.
 *  - Semua alokasi GC arena; reset hanya mengosongkan index.
 * ============================================================ */

#define CANON_INIT_CAP 16
#define CANON_MAX_ENTRIES 1024
#define CANON_MAX_NODES 256     /* batas ukuran subtree per entri */
#define CANON_MAX_STREAM 256    /* batas token ternormalisasi per entri */
#define CANON_BUCKETS 256       /* index hash: scan per lookup O(1) */

typedef struct CanonEntry {
  unsigned long h;      /* FNV-1a utama — penyaring */
  unsigned long h2;     /* FNV-1a seed lain — penyaring kedua */
  int streamType[CANON_MAX_STREAM]; /* tipe token ternormalisasi */
  char *streamValue[CANON_MAX_STREAM]; /* salinan value (gcdup), NULL boleh */
  int streamLen;
  int nodeCount;
  int rootRel;          /* posisi root di nodes[] (reserved duluan) */
  int next;             /* chain bucket (index entri, -1 = akhir) */
  AstNode *nodes;       /* salinan subtree, id relatif 0..count-1 */
} CanonEntry;

static struct {
  CanonEntry *items;
  int count;
  int capacity;
  int buckets[CANON_BUCKETS]; /* head index per bucket, -1 = kosong */
  bool initDone; /* buckets[] diisi -1 lazily (static init = 0 = index sah!) */
} g_canon = {0};

static int g_canonHits = 0;
static int g_canonMisses = 0;

/* Tipe aman di-share: ekspresi murni (tanpa statement/scope). */
static bool canonNodeShareable(const AstNode *n) {
  switch (n->type) {
  case NODE_IDENTIFIER:
  case NODE_LITERAL_ID:
  case NODE_NUMBER:
  case NODE_DECIMAL:
  case NODE_BOOLEAN:
  case NODE_STRING:
  case NODE_MEMBER:
  case NODE_SUBSCRIPT:
  case NODE_CALL:
  case NODE_BINARY:
  case NODE_STRING_INTERP:
    return true;
  default:
    return false;
  }
}

static unsigned long canonHashInit(unsigned long seed) {
  return seed;
}

static unsigned long canonSpanHash(const Token *t, int a, int b, unsigned long seed) {
  unsigned long x = canonHashInit(seed);
  for (int i = a; i < b; i++) {
    const DataToken *d = &t->data[i];
    if (d->type == NEWLINE || d->type == TAB) continue;
    x = (x ^ (unsigned char)d->type) * 1099511628211ULL;
    if (d->value)
      for (const char *s = d->value; *s; s++)
        x = (x ^ (unsigned char)*s) * 1099511628211ULL;
    x = (x ^ 0xFF) * 1099511628211ULL;
  }
  return x;
}

/* Normalisasi span → stream (skip whitespace). Return panjang,
 * -1 bila melebihi kapasitas. */
static int canonNormalize(const Token *t, int a, int b, int *types) {
  int n = 0;
  for (int i = a; i < b; i++) {
    const DataToken *d = &t->data[i];
    if (d->type == NEWLINE || d->type == TAB) continue;
    if (n >= CANON_MAX_STREAM) return -1;
    types[n++] = d->type;
  }
  return n;
}

static void canonTrimSpan(Token *t, int *a, int *b) {
  while (*a < *b && grammarIsWhitespace(t, *a)) (*a)++;
  while (*b > *a && grammarIsWhitespace(t, *b - 1)) (*b)--;
}

/* ---- Capture: DFS reachable-set + salinan relatif ---- */

typedef struct CanonCapture {
  AstNode *out;      /* rel 0..count-1 */
  int *map;          /* oldId -> rel, -1 = belum */
  int count;
  int rootPool;      /* rootId absolut */
} CanonCapture;

static int canonRelOf(CanonCapture *c, const Node *pool, int oldId) {
  if (oldId < 0 || oldId >= pool->length) return -1;
  if (c->map[oldId] >= 0) return c->map[oldId];
  if (c->count >= CANON_MAX_NODES) return -1;

  const AstNode *src = &pool->ast[oldId];
  if (!canonNodeShareable(src)) return -1;

  /* Recurse dulu (child dulu → rel urutan stabil), seperti parser. */
  int relBase = c->count;
  c->map[oldId] = relBase; /* reserve; menahan siklus (tak seharusnya ada) */
  c->count++;

  AstNode dst = *src;
  switch (src->type) {
  case NODE_BINARY: {
    int l = -1, r = -1;
    if (src->binary.left >= 0) {
      l = canonRelOf(c, pool, src->binary.left);
      if (l < 0) return -1;
    }
    /* right == -1 sah: unary not ("!") dari createNot. */
    if (src->binary.right >= 0) {
      r = canonRelOf(c, pool, src->binary.right);
      if (r < 0) return -1;
    }
    dst.binary.left = l;
    dst.binary.right = r;
    dst.binary.op = src->binary.op ? gcdup(src->binary.op) : NULL;
    break;
  }
  case NODE_CALL: {
    int callee = src->call.callee >= 0 ? canonRelOf(c, pool, src->call.callee) : -1;
    if (callee < 0) return -1;
    dst.call.callee = callee;
    if (src->call.length > 0 && src->call.args) {
      int *args = gcmall(sizeof(int) * (size_t)src->call.length);
      if (!args) return -1;
      for (int j = 0; j < src->call.length; j++) {
        int ar = canonRelOf(c, pool, src->call.args[j]);
        if (ar < 0) return -1;
        args[j] = ar;
      }
      dst.call.args = args;
    } else {
      dst.call.args = NULL;
      dst.call.length = 0;
    }
    break;
  }
  case NODE_MEMBER: {
    int o = canonRelOf(c, pool, src->member.object);
    int m = canonRelOf(c, pool, src->member.member);
    if (o < 0 || m < 0) return -1;
    dst.member.object = o;
    dst.member.member = m;
    break;
  }
  case NODE_SUBSCRIPT: {
    int p = canonRelOf(c, pool, src->subscript.posId);
    int ix = canonRelOf(c, pool, src->subscript.index);
    if (p < 0 || ix < 0) return -1;
    dst.subscript.posId = p;
    dst.subscript.index = ix;
    break;
  }
  case NODE_STRING_INTERP: {
    if (src->stringInterp.length > 0 && src->stringInterp.parts) {
      int *parts = gcmall(sizeof(int) * (size_t)src->stringInterp.length);
      if (!parts) return -1;
      for (int j = 0; j < src->stringInterp.length; j++) {
        int pr = canonRelOf(c, pool, src->stringInterp.parts[j]);
        if (pr < 0) return -1;
        parts[j] = pr;
      }
      dst.stringInterp.parts = parts;
    } else {
      dst.stringInterp.parts = NULL;
      dst.stringInterp.length = 0;
    }
    break;
  }
  case NODE_IDENTIFIER:
    dst.identifier.name = src->identifier.name ? gcdup(src->identifier.name) : NULL;
    break;
  case NODE_LITERAL_ID:
  case NODE_STRING:
    dst.string.value = src->string.value ? gcdup(src->string.value) : NULL;
    break;
  case NODE_DECIMAL:
    dst.decimal.lexeme = src->decimal.lexeme ? gcdup(src->decimal.lexeme) : NULL;
    break;
  default:
    break; /* literal payloadless */
  }

  /* Lokasi error milik file requestor — stamp ulang saat install. */
  dst.token = NULL;
  dst.line = 0;
  dst.row = 0;

  c->out[relBase] = dst;
  return relBase;
}

/* ---- Publish ---- */

void syntaxCanonicalPublish(Request *req, int spanA, int spanB, int rootId) {
  if (!req || !req->tokens || !req->node || rootId < 0 || rootId >= req->node->length)
    return;
  if (g_canon.count >= CANON_MAX_ENTRIES) return;

  Token *t = req->tokens;
  int a = spanA, b = spanB;
  canonTrimSpan(t, &a, &b);
  if (a >= b) return;

  /* Lazy init: bucket 0 = "entri index 0" yang sah — tanpa ini, adopt
   * pertama membaca items[0] yang belum dialokasi (segfault). */
  if (!g_canon.initDone) {
    memset(g_canon.buckets, 0xFF, sizeof(g_canon.buckets));
    g_canon.initDone = true;
  }

  int types[CANON_MAX_STREAM];
  int streamLen = canonNormalize(t, a, b, types);
  if (streamLen <= 1) return; /* span 1-token: memo per-Request cukup */

  CanonCapture c;
  c.out = gcmall(sizeof(AstNode) * (size_t)CANON_MAX_NODES);
  c.map = gcmall(sizeof(int) * (size_t)req->node->length);
  if (!c.out || !c.map) return;
  for (int i = 0; i < req->node->length; i++) c.map[i] = -1;
  c.count = 0;
  c.rootPool = rootId;

  int rootRel = canonRelOf(&c, req->node, rootId);
  if (rootRel < 0) {
    gcfree(c.out);
    gcfree(c.map);
    return;
  }

  if (g_canon.count == g_canon.capacity) {
    int cap = g_canon.capacity ? g_canon.capacity * 2 : CANON_INIT_CAP;
    CanonEntry *next = gcrealloc(g_canon.items, sizeof(CanonEntry) * (size_t)cap);
    if (!next) {
      gcfree(c.out);
      gcfree(c.map);
      return;
    }
    g_canon.items = next;
    g_canon.capacity = cap;
  }

  CanonEntry *e = &g_canon.items[g_canon.count];
  memset(e, 0, sizeof(*e));
  e->h = canonSpanHash(t, a, b, 1469598103934665603ULL);
  e->h2 = canonSpanHash(t, a, b, 1469598103934665603ULL ^ 0x9E3779B97F4A7C15ULL);
  e->streamLen = streamLen;
  {
    /* Salin stream token per STREAM (bukan per raw index — whitespace
     * di-skip, jadi indeks stream != indeks span mentah). Type DAN
     * value: tanpa streamType, verify selalu gagal → adopt tak pernah
     * hit. */
    int si = 0;
    for (int k = a; k < b; k++) {
      const DataToken *d = &t->data[k];
      if (d->type == NEWLINE || d->type == TAB) continue;
      e->streamType[si] = d->type;
      e->streamValue[si] = d->value ? gcdup(d->value) : NULL;
      si++;
    }
  }
  e->nodeCount = c.count;
  e->rootRel = rootRel;
  e->nodes = c.out;
  /* Tautkan ke bucket hash — scan adopt per lookup O(1). */
  unsigned long bucket = e->h & (CANON_BUCKETS - 1);
  e->next = g_canon.buckets[bucket];
  g_canon.buckets[bucket] = g_canon.count;
  g_canon.count++;
  gcfree(c.map);
}

/* ---- Adopt: verifikasi stream penuh, lalu install dengan remap ---- */

static int canonInstall(Request *req, const CanonEntry *e) {
  int base = req->node->length;
  for (int rel = 0; rel < e->nodeCount; rel++) {
    AstNode n = e->nodes[rel];
    switch (n.type) {
    case NODE_BINARY:
      if (n.binary.left >= 0) n.binary.left = base + n.binary.left;
      if (n.binary.right >= 0) n.binary.right = base + n.binary.right;
      break;
    case NODE_CALL:
      if (n.call.callee >= 0) n.call.callee = base + n.call.callee;
      if (n.call.args && n.call.length > 0) {
        /* Array args milik ENTRI kanonik — JANGAN di-mutasi in-place
         * (entri di-share; install kedua akan menggeser id yang sudah
         * tergeser → id sampah). Salin ke pool requestor. */
        int *args = gcmall(sizeof(int) * (size_t)n.call.length);
        if (!args) return -1;
        for (int j = 0; j < n.call.length; j++)
          args[j] =
              n.call.args[j] >= 0 ? base + n.call.args[j] : n.call.args[j];
        n.call.args = args;
      }
      break;
    case NODE_MEMBER:
      n.member.object = base + n.member.object;
      n.member.member = base + n.member.member;
      break;
    case NODE_SUBSCRIPT:
      n.subscript.posId = base + n.subscript.posId;
      n.subscript.index = base + n.subscript.index;
      break;
    case NODE_STRING_INTERP:
      if (n.stringInterp.parts && n.stringInterp.length > 0) {
        /* Sama seperti call.args: salin, jangan mutasi array entri. */
        int *parts = gcmall(sizeof(int) * (size_t)n.stringInterp.length);
        if (!parts) return -1;
        for (int j = 0; j < n.stringInterp.length; j++)
          parts[j] = n.stringInterp.parts[j] >= 0
                         ? base + n.stringInterp.parts[j]
                         : n.stringInterp.parts[j];
        n.stringInterp.parts = parts;
      }
      break;
    default:
      break;
    }
    if (createAst(req->node, n) < 0) return -1;
  }
  /* Root di-reserve duluan oleh DFS (rel kecil) — bukan rel terakhir. */
  return base + e->rootRel;
}

int syntaxCanonicalAdopt(Request *req, int spanA, int spanB) {
  if (!req || !req->tokens || !req->node) return -1;
  Token *t = req->tokens;
  int a = spanA, b = spanB;
  canonTrimSpan(t, &a, &b);
  if (a >= b) return -1;

  int types[CANON_MAX_STREAM];
  int streamLen = canonNormalize(t, a, b, types);
  if (streamLen <= 1) return -1; /* span 1-token tak pernah di-publish */

  /* Lazy init juga di adopt: adopt bisa jalan SEBELUM publish pertama. */
  if (!g_canon.initDone) {
    memset(g_canon.buckets, 0xFF, sizeof(g_canon.buckets));
    g_canon.initDone = true;
  }
  if (!g_canon.items) return -1; /* belum ada entri sama sekali */

  /* Hash dihitung SEKALI — biaya scan per entri cuma 3 perbandingan. */
  unsigned long h = canonSpanHash(t, a, b, 1469598103934665603ULL);
  unsigned long h2 =
      canonSpanHash(t, a, b, 1469598103934665603ULL ^ 0x9E3779B97F4A7C15ULL);

  for (int i = g_canon.buckets[h & (CANON_BUCKETS - 1)]; i >= 0;
       i = g_canon.items[i].next) {
    CanonEntry *e = &g_canon.items[i];
    if (e->streamLen != streamLen) continue;
    if (e->h != h || e->h2 != h2) continue;

    /* Verifikasi penuh — stream tersimpan dibandingkan token-per-token
     * terhadap span request (hash hanya penyaring). */
    int si = 0;
    bool equal = true;
    for (int k = a; equal && k < b; k++) {
      const DataToken *d = &t->data[k];
      if (d->type == NEWLINE || d->type == TAB) continue;
      if (e->streamType[si] != d->type) equal = false;
      else if (!e->streamValue[si] != !d->value) equal = false;
      else if (d->value && strcmp(e->streamValue[si], d->value) != 0) equal = false;
      si++;
    }
    if (!equal) continue;

    g_canonHits++;
    return canonInstall(req, e);
  }
  g_canonMisses++;
  return -1;
}

void syntaxCanonicalReset(void) {
  g_canon.count = 0;
  memset(g_canon.buckets, 0xFF, sizeof(g_canon.buckets)); /* -1 semua */
  g_canonHits = 0;
  g_canonMisses = 0;
}

int syntaxCanonicalHits(void) { return g_canonHits; }

int syntaxCanonicalMisses(void) { return g_canonMisses; }
