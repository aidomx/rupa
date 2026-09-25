#include <rupa.h>

/* Forward — index hash per-Env (didefinisikan di bawah semCreateEnv). */
static void envHashInsert(RuntimeEnv *env, RuntimeBinding *b);

/* ===== Name interning (profil callgrind: strcmp+semFind ≈ 40% Ir loop
 * panas — semFind jalan linked-list bindings dengan strcmp per simpul).
 * Setiap nama dikanonikalisasi ke SATU instance malloc (lifetime proses):
 * semua binding menunjuk instance yang sama sehingga perbandingan cukup
 * b->name == name (pointer). Fallback strcmp tetap ada untuk jalur tanpa
 * intern (allocation gagal). Tabel dibaca tanpa lock setelah insert —
 * insert hanya di jalur deklarasi/definisi, bukan loop eksekusi. */

struct NameEntry {
  const char *s;
  struct NameEntry *next;
};

#define SEM_NAME_BUCKETS 1024 /* power of two */

static struct NameEntry *g_sem_names[SEM_NAME_BUCKETS];

static size_t semNameHash(const char *s) {
  size_t x = 1469598103934665603ULL; /* FNV-1a */
  for (; *s; s++) {
    x ^= (unsigned char)*s;
    x *= 1099511628211ULL;
  }
  return x;
}

/* Instance kanonik nama — panggilan berikutnya dengan isi sama
 * mengembalikan pointer yang sama (perbandingan O(1)). */
static const char *semIntern(const char *name) {
  if (!name) return NULL;
  size_t b = semNameHash(name) & (SEM_NAME_BUCKETS - 1);
  for (struct NameEntry *e = g_sem_names[b]; e; e = e->next)
    if (!strcmp(e->s, name)) return e->s;

  struct NameEntry *e = malloc(sizeof(*e));
  if (!e) return name;
  size_t len = strlen(name) + 1;
  char *dup = malloc(len);
  if (!dup) {
    free(e);
    return name;
  }
  memcpy(dup, name, len);
  e->s = dup;
  e->next = g_sem_names[b];
  g_sem_names[b] = e;
  return dup;
}

RuntimeEnv *semCreateEnv(RuntimeEnv *parent) {
  RuntimeEnv *env = gccalloc(1, sizeof(*env));
  if (env) env->parent = parent;
  return env;
}

/* ===== Hash index per-Env =====
 * Profil callgrind: semFind = linked-list scan dengan strcmp per simpul
 * ≈ 40% Ir loop panas. Index insert-only (binding tak pernah dihapus)
 * memungkinkan open addressing TANPA tombstone. Karena semua b->name
 * di-intern (pointer kanonik), hash dihitung dari pointer — O(1) murni,
 * tanpa strcmp di jalur lookup. */

static unsigned long semPtrHash(const void *p) {
  unsigned long x = (unsigned long)(uintptr_t)p >> 4;
  x *= 0x9E3779B97F4A7C15UL;
  x ^= x >> 29;
  return x;
}

/* ===== Jalur cepat berbasis nama kanonik =====
 * IRValue (ir.c) men-cache (canon, hash) sekali saat build IR; mesin IR
 * (execute.c) memakai varian *Canon di bawah sehingga lookup loop panas
 * melompati interning + FNV string per iterasi. */

const char *semNameCanonical(const char *name) { return semIntern(name); }

unsigned long semNameHashOf(const void *canon) { return semPtrHash(canon); }

static void envHashGrow(RuntimeEnv *env, int want_cap) {
  unsigned long *tab = gccalloc((size_t)want_cap, sizeof(unsigned long));
  if (!tab) return; /* index gagal: fallback strcmp tetap benar */
  if (env->hash) gcfree(env->hash);
  env->hash = tab;
  env->hashMask = (unsigned long)want_cap - 1;
  env->hashCount = 0;
  for (RuntimeBinding *b = env->bindings; b; b = b->next)
    envHashInsert(env, b);
}

/* Insert binding ke index env. Dipanggil dari semDeclare; binding
 * sudah di-head-of-list sebelum insert. */
static void envHashInsert(RuntimeEnv *env, RuntimeBinding *b) {
  if (!env || !b || !b->name) return;
  if (!env->hash || (env->hashCount + 1) * 4 > (long)(env->hashMask + 1) * 3) {
    int cap = env->hash ? (int)(env->hashMask + 1) * 2 : 16;
    envHashGrow(env, cap);
    if (!env->hash) return; /* tanpa index: lookup fallback strcmp */
  }
  unsigned long mask = env->hashMask;
  unsigned long i = semPtrHash(b->name) & mask;
  while (env->hash[i] != 0) i = (i + 1) & mask;
  env->hash[i] = (unsigned long)(uintptr_t)b + 1;
  env->hashCount++;
}

/* Inti lookup hash-index: env ber-index, canon + hash sudah siap. */
static RuntimeBinding *envHashFind(const RuntimeEnv *env, const char *canon, unsigned long h) {
  unsigned long mask = env->hashMask;
  for (unsigned long i = h & mask; env->hash[i] != 0; i = (i + 1) & mask) {
    RuntimeBinding *b = (RuntimeBinding *)(uintptr_t)(env->hash[i] - 1);
    if (b->name == canon) return b;
  }
  return NULL;
}

RuntimeBinding *semFindLocal(RuntimeEnv *env, const char *name) {
  if (!env || !name) return NULL;
  const char *canon = semIntern(name);
  if (env->hash) return envHashFind(env, canon, semPtrHash(canon));
  for (RuntimeBinding *b = env->bindings; b; b = b->next)
    if (b->name == canon || !strcmp(b->name, name)) return b;
  return NULL;
}

/* Varian canon: penelepon memegang pointer kanonik + hash-nya (cache
 * IRValue) — tanpa intern, tanpa hash string. Binding selalu dibuat
 * lewat semDeclare (nama di-intern) sehingga perbandingan pointer
 * cukup bahkan di env tanpa index. */
RuntimeBinding *semFindLocalCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash) {
  if (!env || !canon) return NULL;
  if (env->hash) return envHashFind(env, canon, canonHash);
  for (RuntimeBinding *b = env->bindings; b; b = b->next)
    if (b->name == canon) return b;
  return NULL;
}

RuntimeBinding *semFind(RuntimeEnv *env, const char *name) {
  if (!name) return NULL;
  const char *canon = semIntern(name);
  if (!canon) return NULL;
  unsigned long h = semPtrHash(canon);
  for (; env; env = env->parent) {
    if (env->hash) {
      RuntimeBinding *b = envHashFind(env, canon, h);
      if (b) return b;
      continue; /* env ini tidak punya — lanjut parent */
    }
    for (RuntimeBinding *b = env->bindings; b; b = b->next)
      if (b->name == canon || !strcmp(b->name, name)) return b;
  }
  return NULL;
}

RuntimeBinding *semFindCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash) {
  if (!canon) return NULL;
  for (; env; env = env->parent) {
    if (env->hash) {
      RuntimeBinding *b = envHashFind(env, canon, canonHash);
      if (b) return b;
      continue;
    }
    for (RuntimeBinding *b = env->bindings; b; b = b->next)
      if (b->name == canon) return b;
  }
  return NULL;
}

bool semDeclare(RuntimeEnv *env, const char *name, const char *type) {
  if (!env || !name) return false;

  RuntimeBinding *b = semFindLocal(env, name);
  if (b) {
    if (type && !b->type) b->type = gcstrdup(type);
    return true;
  }

  b = gccalloc(1, sizeof(*b));
  if (!b) return false;

  /* Intern: salin dari instance kanonik — pointer binding antar env
   * konsisten sehingga semFind cukup bandingkan pointer. */
  b->name = (char *)semIntern(name);
  b->type = type ? gcstrdup(type) : NULL;
  b->value = valueNull();
  b->next = env->bindings;
  env->bindings = b;
  envHashInsert(env, b);
  env->isRepl = false;
  return true;
}

void semSet(RuntimeEnv *env, const char *name, RuntimeValue value) {
  if (!env || !name) return;

  RuntimeBinding *b = semFindLocal(env, name);
  if (!b) {
    if (!semDeclare(env, name, NULL)) return;
    b = semFindLocal(env, name);
  }
  if (b) b->value = value;
}

/* Tandai binding sebagai hasil `import` — hidden di whole-env export
 * (loader.c) kecuali opt-in via policy `-> { import: public }`. */
void semMarkImport(RuntimeEnv *env, const char *name) {
  if (!env || !name) return;
  RuntimeBinding *b = semFindLocal(env, name);
  if (b) b->isImport = true;
}

/* Baca flag isImport; bila binding tidak ada kembalikan false. */
bool semIsImport(const RuntimeEnv *env, const char *name) {
  if (!env || !name) return false;
  const RuntimeBinding *b = semFindLocal((RuntimeEnv *)env, name);
  return b ? b->isImport : false;
}

bool semSetConst(RuntimeEnv *env, const char *name, RuntimeValue value) {
  if (!env || !name) return false;

  RuntimeBinding *b = semFindLocal(env, name);
  if (!b) {
    if (!semDeclare(env, name, NULL)) return false;
    b = semFindLocal(env, name);
  }
  if (!b) return false;
  /* Deklarasi const selalu menulis + mengunci ulang: re-init sah di
   * setiap iterasi loop / call fungsi (slot lama di-reset). */
  b->isConst = true;
  b->value = value;
  return true;
}

bool semIsConst(RuntimeEnv *env, const char *name) {
  if (!env || !name) return false;
  RuntimeBinding *b = semFind(env, name);
  return b ? b->isConst : false;
}

const char *semType(RuntimeEnv *env, const char *name) {
  RuntimeBinding *b = semFind(env, name);
  return b ? b->type : NULL;
}

bool semGet(RuntimeEnv *env, const char *name, RuntimeValue *out) {
  RuntimeBinding *b = semFind(env, name);
  if (b) {
    if (out) *out = b->value;
    return true;
  }
  return false;
}

/* ===== Varian canon (nama kanonik + hash precomputed) ===== */

bool semGetCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash, RuntimeValue *out) {
  RuntimeBinding *b = semFindCanon(env, canon, canonHash);
  if (b) {
    if (out) *out = b->value;
    return true;
  }
  return false;
}

void semSetCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash, RuntimeValue value) {
  if (!env || !canon) return;
  RuntimeBinding *b = semFindLocalCanon(env, canon, canonHash);
  if (!b) {
    /* Belum ada: jalur umum (intern idempoten untuk pointer kanonik). */
    semSet(env, canon, value);
    return;
  }
  b->value = value;
}

bool semIsConstCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash) {
  RuntimeBinding *b = semFindCanon(env, canon, canonHash);
  return b ? b->isConst : false;
}

bool semSetConstCanon(RuntimeEnv *env, const char *canon, unsigned long canonHash,
                      RuntimeValue value) {
  if (!env || !canon) return false;
  RuntimeBinding *b = semFindLocalCanon(env, canon, canonHash);
  if (!b) {
    if (!semDeclare(env, canon, NULL)) return false;
    b = semFindLocalCanon(env, canon, canonHash);
  }
  if (!b) return false;
  /* Sejajar semSetConst: deklarasi const selalu menulis + mengunci. */
  b->isConst = true;
  b->value = value;
  return true;
}
