#include <rupa.h>
/* setelah rupa.h: guard-nya bergantung RUPA_PACKAGE_H */
#include "module.h"

/* loader_state.c — state global loader modul: path sumber aktif, event
 * loop, guard circular import (module stack), dan module cache.
 * Unit hasil split loader.c (rules.md: modular, jangan membengkak):
 *   loader_path.c    — utilitas & resolusi path import
 *   loader_state.c   — state global + module cache (file ini)
 *   loader_export.c  — pembangunan object export entries
 *   loader.c         — entry: loadModuleFile/loadModuleFileError
 */

/* ---- Global state ---- */
struct EventLoop *g_event_loop = NULL;
const char *g_source_file_path = NULL;

/* Path skrip entry point (file yang dijalankan rupa, serve, go, atau
 * test). Hanya runner-level yang memanggil setSourceFilePath — loader
 * modul tidak — sehingga nilainya tidak pernah tertukar oleh import. */
static const char *g_main_script_path = NULL;

void setSourceFilePath(const char *path) {
  g_source_file_path = path;
  if (path)
    g_main_script_path = path;
}
const char *getSourceFilePath(void) {
  return g_source_file_path;
}
const char *getMainScriptPath(void) {
  return g_main_script_path;
}
struct EventLoop *getEventLoop(void) {
  return g_event_loop;
}

/* ---- Circular import detection ----
 * Stack canonical path dari modul yang SEDANG dimuat (belum selesai
 * dieksekusi). Import yang menunjuk file di stack ini = circular.
 * Tanpa guard, `import x from ./x` (modul meng-import dirinya sendiri)
 * memanggil loadModuleFile tanpa batas: baca → parse → eksekusi →
 * import lagi — proses hang (unbounded recursion).
 * Catatan: tidak thread-safe, sejalan dengan global g_* lain yang
 * diasumsikan dipakai single-thread pada module loading. */
struct ModuleFrame {
  char *path; /* canonical path (RUPA_REALPATH), GC-managed */
  struct ModuleFrame *next;
};
static struct ModuleFrame *g_module_stack = NULL;

bool moduleIsLoading(const char *canonical) {
  for (struct ModuleFrame *f = g_module_stack; f; f = f->next)
    if (strcmp(f->path, canonical) == 0) return true;
  return false;
}

/* Kepemilikan canonical pindah ke frame; dibebaskan saat pop. */
void modulePushLoading(char *canonical_owned) {
  struct ModuleFrame *f = gccalloc(1, sizeof(*f));
  if (!f) {
    gcfree(canonical_owned);
    return;
  }
  f->path = canonical_owned;
  f->next = g_module_stack;
  g_module_stack = f;
}

void modulePopLoading(void) {
  struct ModuleFrame *f = g_module_stack;
  if (!f) return;
  g_module_stack = f->next;
  gcfree(f->path);
  gcfree(f);
}

/* Teruskan hanya error struktural modul (ModuleError — mis. circular
 * import yang terdeteksi di level lebih dalam) dari error lokal
 * eksekusi modul ke error caller. Error runtime biasa di dalam modul
 * tetap tidak di-propagasi — perilaku lama dipertahankan. */
void propagateModuleErrors(Error *dst, Error *src) {
  if (!dst || !src) return;
  for (int i = 0; i < src->size; i++) {
    if (src->info[i].code && strcmp(src->info[i].code, "ModuleError") == 0)
      addError(dst, src->info[i]);
  }
}

/* ---- Module cache ----
 * Hasil load per canonical path di-memoize: module yang sama hanya
 * dieksekusi SEKALI per run program. Tanpa ini, satu ExportDecl
 * diproses dua kali (interpreter-side + loader re-export) dan setiap
 * consumer me-load ulang seluruh sub-tree — di package bertingkat
 * redundansi mengganda per level hingga eksekusi meledak (hang).
 * Cache diisi hanya untuk module yang SELESAI dimuat; cycle tetap
 * ditangani module-stack, bukan cache. */
struct ModuleCacheEntry {
  char *path; /* canonical, GC-managed */
  RuntimeValue value;
};
static struct ModuleCacheEntry *g_module_cache = NULL;
static int g_module_cache_count = 0;
static int g_module_cache_capacity = 0;

bool moduleCacheGet(const char *canonical, RuntimeValue *out) {
  for (int i = 0; i < g_module_cache_count; i++)
    if (strcmp(g_module_cache[i].path, canonical) == 0) {
      *out = g_module_cache[i].value;
      return true;
    }
  return false;
}

/* Kepemilikan `canonical_owned` (gcstrdup) pindah ke cache. */
void moduleCachePut(char *canonical_owned, RuntimeValue v) {
  if (g_module_cache_count >= g_module_cache_capacity) {
    int ncap = g_module_cache_capacity ? g_module_cache_capacity * 2 : 16;
    struct ModuleCacheEntry *nc = gcmall(ncap * sizeof(*nc));
    if (!nc) {
      gcfree(canonical_owned);
      return;
    }
    if (g_module_cache) memcpy(nc, g_module_cache, g_module_cache_count * sizeof(*nc));
    g_module_cache = nc;
    g_module_cache_capacity = ncap;
  }
  g_module_cache[g_module_cache_count].path = canonical_owned;
  g_module_cache[g_module_cache_count].value = v;
  g_module_cache_count++;
}

/* Reset cache module (dipanggil serve.c tiap reload — proses dev server
 * panjang umur & serveExecScript() dieksekusi berkali-kali dalam SATU
 * proses, jadi cache lama harus dibuang atau import di serve.rp/main.rp
 * akan selalu mengembalikan versi module yang di-load pertama kali,
 * bukan yang baru diedit). Isi array GC-managed, cukup lupakan (count=0)
 * — tanpa free manual, selaras pola GC arena di file ini. */
void moduleCacheReset(void) { g_module_cache_count = 0; }

int moduleCacheCount(void) { return g_module_cache_count; }

/* Bangun object whole-env dari bindings module — dipakai jalur whole-env
 * normal DAN pre-redirect cache (tree export). Binding hasil `import`
 * (isImport) selalu hidden: bukan milik module.
 *
 * Export surface `pub` (design fn/namespace): bila env punya binding
 * `pub` (hasPub), HANYA binding pub yang diterbitkan — default private.
 * Member private diterbitkan sebagai null + metadata `_private` agar
 * akses dari luar menghasilkan PrivateError (pola policy export lama).
 * File tanpa `pub` sama sekali = perilaku lama (semua ikut). */
RuntimeValue buildWholeEnvValue(RuntimeEnv *env) {
  struct RuntimeObjectEntry *entries = NULL;
  struct RuntimeObjectEntry *priv_entries = NULL;
  bool pubMode = semHasPub(env);
  for (RuntimeBinding *b = env->bindings; b; b = b->next) {
    if (strcmp(b->name, "AWAIT") == 0 || strcmp(b->name, "SUCCESS") == 0 ||
        strcmp(b->name, "ERROR") == 0)
      continue;
    if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
    if (b->isImport) continue;
    if (pubMode && !b->isPub) {
      /* Private: terbitkan null di surface + catat di _private — akses
       * luar = PrivateError (member_get menemukan null lalu melihat
       * _private; selaras pola policy export lama modFilterPolicies). */
      struct RuntimeObjectEntry *p = gccalloc(1, sizeof(*p));
      p->key = gcstrdup(b->name);
      p->value = valueNull();
      p->next = priv_entries;
      priv_entries = p;

      struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
      e->key = gcstrdup(b->name);
      e->value = valueNull();
      e->next = entries;
      entries = e;
      continue;
    }
    struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
    e->key = gcstrdup(b->name);
    e->value = b->value;
    e->next = entries;
    entries = e;
  }
  if (pubMode && priv_entries) {
    struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
    pe->key = gcstrdup("_private");
    pe->value = valueObject(priv_entries);
    pe->next = entries;
    entries = pe;
  }
  return valueObject(entries);
}
