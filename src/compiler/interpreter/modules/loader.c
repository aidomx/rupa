#include <rupa.h>
/* setelah rupa.h: guard-nya bergantung RUPA_PACKAGE_H */
#include "module.h"
/* Module Loader — utilities for loading and executing .rp files as modules.
 * The interpreter dispatcher (interpretNode) is in dispatch.c. */

/* ---- Global state ---- */
struct EventLoop *g_event_loop = NULL;
const char *g_source_file_path = NULL;

void setSourceFilePath(const char *path) {
  g_source_file_path = path;
}
const char *getSourceFilePath(void) {
  return g_source_file_path;
}
struct EventLoop *getEventLoop(void) {
  return g_event_loop;
}

/* ---- Internal utility functions (static) ---- */

static bool fileExists(const char *path) {
  if (!path) return false;
  FILE *f = fopen(path, "rb");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

static char *resolveModulePath(const char *module_path) {
  if (!module_path) return NULL;
  const char *path = module_path;
  if (path[0] == '.' && path[1] == '/') path += 2;
  int len = (int)strlen(path);
  char *buf = malloc(len + 4);
  if (!buf) return NULL;
  for (int i = 0; i < len; i++)
    buf[i] = path[i] == '.' ? '/' : path[i];
  buf[len] = '\0';
  strcat(buf, ".rp");
  return buf;
}

static char *joinPath(const char *dir, const char *rel) {
  if (!dir || !rel) return rel ? strdup(rel) : NULL;
  int dlen = (int)strlen(dir);
  int rlen = (int)strlen(rel);
  int need_sep = (dlen > 0 && dir[dlen - 1] != '/');
  char *buf = malloc(dlen + need_sep + rlen + 1);
  if (!buf) return NULL;
  memcpy(buf, dir, dlen);
  if (need_sep) buf[dlen] = '/';
  memcpy(buf + dlen + need_sep, rel, rlen + 1);
  return buf;
}

static char *dirName(const char *path) {
  if (!path) return NULL;
  const char *last_slash = strrchr(path, '/');
  if (!last_slash) return strdup(".");
  int len = (int)(last_slash - path);
  if (len == 0) return strdup("/");
  char *buf = malloc(len + 1);
  if (!buf) return NULL;
  memcpy(buf, path, len);
  buf[len] = '\0';
  return buf;
}

bool hasDotSlash(const char *path) {
  if (!path || path[0] != '.') return false;
  if (path[1] == '/') return true;
  if (path[1] == '.' && path[2] == '/') return true;
  return false;
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

/* Path kanonik untuk identitas modul: realpath menyelesaikan symlink
 * dan '..' sehingga "./rpx" dan "rpx" dari source dir yang sama
 * menghasilkan string identik. File yang belum ada fallback ke path
 * apa adanya (readfile akan gagal nanti seperti biasa). */
static char *moduleCanonicalPath(const char *path) {
  if (!path) return NULL;
  char *resolved = RUPA_REALPATH(path);
  if (!resolved) return gcstrdup(path); /* best effort */
  char *out = gcstrdup(resolved);
  free(resolved);
  return out;
}

static bool moduleIsLoading(const char *canonical) {
  for (struct ModuleFrame *f = g_module_stack; f; f = f->next)
    if (strcmp(f->path, canonical) == 0) return true;
  return false;
}

/* Kepemilikan canonical pindah ke frame; dibebaskan saat pop. */
static void modulePushLoading(char *canonical_owned) {
  struct ModuleFrame *f = gccalloc(1, sizeof(*f));
  if (!f) {
    gcfree(canonical_owned);
    return;
  }
  f->path = canonical_owned;
  f->next = g_module_stack;
  g_module_stack = f;
}

static void modulePopLoading(void) {
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
static void propagateModuleErrors(Error *dst, Error *src) {
  if (!dst || !src) return;
  for (int i = 0; i < src->size; i++) {
    if (src->info[i].code && strcmp(src->info[i].code, "ModuleError") == 0)
      addError(dst, src->info[i]);
  }
}

/* Apakah file punya statement export eksplisit? Scan tekstual ringan —
 * gate resolusi ie.txt import #5: Z-self hanya dihitung bila Z mengekspor
 * dirinya ("leaf polos tanpa export tidak dihitung → harus via Y").
 * `export *` dan `namespace` sama-sama dihitung (setara has_export di
 * loader); baris komentar // dan # di-skip. */
static bool fileHasExportStatement(const char *path) {
  if (!path) return false;
  FILE *f = fopen(path, "r");
  if (!f) return false;
  char line[512];
  bool found = false;
  while (!found && fgets(line, sizeof(line), f)) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (p[0] == '/' && p[1] == '/') continue;
    if (p[0] == '#') continue;
    bool is_export = strncmp(p, "export", 6) == 0 &&
                     (p[6] == '\0' || p[6] == ' ' || p[6] == '\t' ||
                      p[6] == '*' || p[6] == '\n' || p[6] == '\r');
    bool is_namespace = strncmp(p, "namespace", 9) == 0 &&
                        (p[9] == '\0' || p[9] == ' ' || p[9] == '\t');
    if (is_export || is_namespace) found = true;
  }
  fclose(f);
  return found;
}

static char *resolveDotPath(const char *module_path, const char *source_dir) {
  if (!module_path || !source_dir) return NULL;

  /* Dual resolution (design/ie.txt import #5): dotted path dicoba sebagai
   * file/module nyata DULU — Z yang mengekspor dirinya sendiri membuat
   * X.Y hanya referensi path — baru fallback ke parent index (Z menjadi
   * sub-module via export Y). Keduanya tidak ada → NULL → ImportError. */
  const char *p = module_path;
  while (p[0] == '.' && p[1] == '.' && p[2] == '/') p += 3;
  if (p[0] == '.' && p[1] == '/') p += 2;
  const char *lastDot = strrchr(p, '.');
  if (lastDot && lastDot[1] != '\0' && lastDot[1] != '/') {
    size_t plen = (size_t)(lastDot - module_path); /* prefix + segmen parent */
    char *dirpart = malloc(plen + 1);
    if (!dirpart) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < plen; i++) {
      char c = module_path[i];
      if (c == '.' && i + 1 < plen && module_path[i + 1] == '.') {
        dirpart[j++] = '.';
        dirpart[j++] = '.';
        i++;
      } else if (c == '.') {
        dirpart[j++] = '/';
      } else {
        dirpart[j++] = c;
      }
    }
    while (j > 0 && dirpart[j - 1] == '/') j--; /* buang '/' ekor sebelum gabung */
    dirpart[j] = '\0';
    /* Nama leaf = segmen terakhir setelah dot terakhir (Z pada X.Y.Z). */
    const char *leaf = lastDot + 1;
    size_t llen = strlen(leaf);

    /* 1) Z-self sebagai file: ./X.Y/Z.rp — leaf mengekspor dirinya. */
    char *leaf_rel = malloc(j + 1 + llen + 4);
    if (leaf_rel) {
      memcpy(leaf_rel, dirpart, j);
      leaf_rel[j] = '/';
      memcpy(leaf_rel + j + 1, leaf, llen);
      strcpy(leaf_rel + j + 1 + llen, ".rp");
      char *full = joinPath(source_dir, leaf_rel);
      free(leaf_rel);
      /* ie.txt #5: file ada TAPI tanpa statement export → bukan Z-self,
       * lanjut ke tree navigation (parent index). */
      if (full && fileExists(full) && fileHasExportStatement(full)) {
        free(dirpart);
        return full;
      }
      free(full);
    }

    /* 2) Z-self sebagai folder: ./X.Y/Z/index.rp. */
    char *leafdir_rel = malloc(j + 1 + llen + 10);
    if (leafdir_rel) {
      memcpy(leafdir_rel, dirpart, j);
      leafdir_rel[j] = '/';
      memcpy(leafdir_rel + j + 1, leaf, llen);
      strcpy(leafdir_rel + j + 1 + llen, "/index.rp");
      char *full = joinPath(source_dir, leafdir_rel);
      free(leafdir_rel);
      if (full && fileExists(full) && fileHasExportStatement(full)) {
        free(dirpart);
        return full;
      }
      free(full);
    }

    /* 3) Tree navigation: ./X.Y/index.rp — Z sub-module via export parent. */
    char *idx_rel = malloc(j + 10);
    if (!idx_rel) {
      free(dirpart);
      return NULL;
    }
    memcpy(idx_rel, dirpart, j);
    idx_rel[j] = '\0';
    strcat(idx_rel, "/index.rp");
    char *full = joinPath(source_dir, idx_rel);
    free(dirpart);
    free(idx_rel);
    if (full && fileExists(full)) return full;
    free(full);
    return NULL;
  }

  int mlen = (int)strlen(module_path);
  char *rel = malloc(mlen + 16);
  if (!rel) return NULL;

  int j = 0;
  for (int i = 0; i < mlen; i++) {
    if (module_path[i] == '.' && i + 1 < mlen && module_path[i + 1] == '.') {
      rel[j++] = '.';
      rel[j++] = '.';
      i++;
    } else if (module_path[i] == '.') {
      rel[j++] = '/';
    } else {
      rel[j++] = module_path[i];
    }
  }
  rel[j] = '\0';

  char *file_rel = malloc(j + 4);
  if (file_rel) {
    memcpy(file_rel, rel, j);
    file_rel[j] = '\0';
    strcat(file_rel, ".rp");
    char *full = joinPath(source_dir, file_rel);
    free(file_rel);
    if (full && fileExists(full)) {
      free(rel);
      return full;
    }
    free(full);
  }

  char *dir_rel = malloc(j + 14);
  if (dir_rel) {
    memcpy(dir_rel, rel, j);
    dir_rel[j] = '\0';
    strcat(dir_rel, "/index.rp");
    char *full = joinPath(source_dir, dir_rel);
    free(dir_rel);
    if (full && fileExists(full)) {
      free(rel);
      return full;
    }
    free(full);
  }

  free(rel);
  return NULL;
}

/* ---- Package boundary ---- */

/* Dir berisi index.rp? */
static bool dirHasIndex(const char *dir) {
  if (!dir || !*dir) return false;
  char *p = joinPath(dir, "index.rp");
  if (!p) return false;
  bool ok = fileExists(p);
  free(p);
  return ok;
}

/* Package root terluar yang memuat file: naik dari dir file, ingat
 * ancestor terjauh yang punya index.rp; berhenti di gap pertama
 * SETELAH package ditemukan. NULL bila file di luar package manapun. */
static char *packageRootOf(const char *file_path) {
  if (!file_path || !*file_path) return NULL;
  char *outermost = NULL;
  char *dir = dirName(file_path);
  for (int depth = 0; dir && depth < 32; depth++) {
    char *parent = dirName(dir);
    bool atRoot = !parent || strcmp(parent, dir) == 0;
    if (dirHasIndex(dir)) {
      free(outermost);
      outermost = dir; /* kepemilikan pindah ke outermost */
    } else {
      free(dir);
      if (outermost) { /* gap setelah package — stop */
        free(parent);
        break;
      }
    }
    if (atRoot) {
      free(parent);
      break;
    }
    dir = parent;
  }
  return outermost;
}

/* Apakah dotted path ini resolve ke FILE leaf sungguhan — bukan index
 * parent dan bukan leaf di dalam package (yang akan di-redirect loader
 * ke parent-nya)? Dipakai dispatch untuk memilih semantik ie.txt:
 * leaf → single entry boleh otomatis namespace (#4); via index parent →
 * member wajib ada (tree nav #5). */
bool modSourceResolvesLeaf(const char *module_path) {
  if (!module_path || !hasDotSlash(module_path)) return false;
  char *source_dir = dirName(g_source_file_path);
  if (!source_dir) return false;
  char *full = resolveDotPath(module_path, source_dir);
  free(source_dir);
  if (!full) return false;
  size_t len = strlen(full);
  bool is_leaf = !(len >= 8 && strcmp(full + len - 8, "/index.rp") == 0);
  if (is_leaf) {
    /* Leaf di dalam package: loader hanya me-redirect bila caller di
     * LUAR package. Caller se-package (mis. open.rp → ./dsn) = muat
     * langsung → semantik leaf. Caller luar → redirect parent index. */
    char *pkg_root = packageRootOf(full);
    if (pkg_root) {
      char *caller = moduleCanonicalPath(g_source_file_path);
      if (!caller) {
        is_leaf = false; /* tanpa konteks caller: anggap redirect */
      } else {
        size_t plen = strlen(pkg_root);
        bool inside = strncmp(caller, pkg_root, plen) == 0 &&
                      (caller[plen] == '/' || caller[plen] == '\0');
        is_leaf = inside;
        gcfree(caller);
      }
      free(pkg_root);
    }
  }
  free(full);
  return is_leaf;
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

static bool moduleCacheGet(const char *canonical, RuntimeValue *out) {
  for (int i = 0; i < g_module_cache_count; i++)
    if (strcmp(g_module_cache[i].path, canonical) == 0) {
      *out = g_module_cache[i].value;
      return true;
    }
  return false;
}

/* Kepemilikan `canonical_owned` (gcstrdup) pindah ke cache. */
static void moduleCachePut(char *canonical_owned, RuntimeValue v) {
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

/* ---- Public API ---- */

/* Bangun object whole-env dari bindings module — dipakai jalur whole-env
 * normal DAN pre-redirect cache (tree export). Binding hasil `import`
 * (isImport) selalu hidden: bukan milik module. */
static RuntimeValue buildWholeEnvValue(RuntimeEnv *env) {
  struct RuntimeObjectEntry *entries = NULL;
  for (RuntimeBinding *b = env->bindings; b; b = b->next) {
    if (strcmp(b->name, "AWAIT") == 0 || strcmp(b->name, "SUCCESS") == 0 ||
        strcmp(b->name, "ERROR") == 0)
      continue;
    if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
    if (b->isImport) continue;
    struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
    e->key = gcstrdup(b->name);
    e->value = b->value;
    e->next = entries;
    entries = e;
  }
  return valueObject(entries);
}

/* Kompat: loader tanpa propagasi error ke caller. */
RuntimeValue loadModuleFile(const char *module_path, bool require_export) {
  return loadModuleFileError(module_path, require_export, NULL);
}

/* Muat & eksekusi file .rp sebagai modul. Deteksi circular import
 * berbasis path kanonik (RUPA_REALPATH): modul yang masih dalam proses
 * dimuat dan di-import lagi menghasilkan ModuleError — bukan hang.
 * `error` opsional (NULL = error tidak dilaporkan ke sistem error). */
RuntimeValue loadModuleFileError(const char *module_path, bool require_export, Error *error) {
  if (!module_path) return valueNull();

  char *full_path = NULL;

  if (module_path[0] == '/') {
    full_path = strdup(module_path);
  } else {
    char *source_dir = dirName(g_source_file_path);
    if (hasDotSlash(module_path)) {
      full_path = resolveDotPath(module_path, source_dir);
    } else if (strchr(module_path, '/') && strlen(module_path) > 3 &&
               strcmp(module_path + strlen(module_path) - 3, ".rp") == 0) {
      if (strncmp(module_path, "stdlib/", 7) == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
          full_path = joinPath(cwd, module_path);
        } else {
          full_path = strdup(module_path);
        }
      } else {
        full_path = joinPath(source_dir, module_path);
      }
    } else {
      char *rel_path = resolveModulePath(module_path);
      if (rel_path) {
        full_path = joinPath(source_dir, rel_path);
        free(rel_path);
      }
    }
    free(source_dir);
  }

  if (!full_path) {
    return valueNull();
  }

  /* Path kanonik = identitas modul untuk guard cycle. full_path
   * (malloc) hanya dibutuhkan sementara — selanjutnya pakai canonical. */
  char *canonical = moduleCanonicalPath(full_path);
  free(full_path);
  full_path = NULL;
  if (!canonical) return valueNull();

  /* Caller path — capture SEKARANG: g_source_file_path diganti ke path
   * module sendiri setelah mod_env dibuat. */
  const char *caller_path = g_source_file_path;

  /* Sudah pernah dimuat selesai? Return hasil yang sama (memoize). */
  RuntimeValue cached;
  if (moduleCacheGet(canonical, &cached)) {
    gcfree(canonical);
    return cached;
  }

  /* Snapshot lokasi error caller — eksekusi module dalam akan menimpa
   * lokasi global; kembalikan sebelum melaporkan ImportError supaya
   * error menunjuk statement import, bukan baris terakhir module. */
  int caller_line = 0, caller_row = 0;
  getRuntimeErrorLocation(&caller_line, &caller_row);

  /* Package boundary (bug rpx_engine): file di dalam package (dir dengan
   * index.rp di rantai ancestor-nya) hanya bisa di-import dari LUAR
   * package bila file itu punya export sendiri — tanpa itu loader
   * return null (docs/syntax/export.md: "Module tanpa export: nothing
   * is exported"). Import dari dalam package (index re-export, sesama
   * file internal) tetap whole-env. Caller NULL (REPL) = jalur lama. */
  bool cross_boundary = false;
  {
    char *pkg_root = packageRootOf(canonical);
    if (pkg_root) {
      if (caller_path && *caller_path) {
        /* caller_path bisa relatif (argv program utama) — kanonikalkan
         * dulu agar perbandingan prefix dengan pkg_root valid. */
        char *caller_canon = moduleCanonicalPath(caller_path);
        if (caller_canon) {
          size_t plen = strlen(pkg_root);
          bool inside = strncmp(caller_canon, pkg_root, plen) == 0 &&
                        (caller_canon[plen] == '/' || caller_canon[plen] == '\0');
          cross_boundary = !inside;
          gcfree(caller_canon);
        }
      }
      free(pkg_root);
    }
  }

  if (moduleIsLoading(canonical)) {
    if (error) {
      static char message[MAX_MESSAGE_LENGTH];
      snprintf(message, sizeof(message), "Circular import detected: '%s' is already being loaded",
               canonical);
      addError(error, (ErrorInfo){.file = canonical,
                                  .code = "ModuleError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR});
    }
    gcfree(canonical);
    return valueNull();
  }
  modulePushLoading(canonical);

  State *state = createGlobalState(10, false);
  if (!state || !state->buffer) {
    modulePopLoading();
    return valueNull();
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  if (!readfile(canonical, buffer)) {
    modulePopLoading();
    return valueNull();
  }

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0) {
    modulePopLoading();
    return valueNull();
  }

  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);
  /* Error lokal eksekusi modul — terpisah dari error caller supaya
   * perilaku lama (error runtime di dalam modul tidak menyeret program
   * utama) tetap terjaga; hanya ModuleError yang di-propagasi. */
  Error *mod_error = createError(10);

  if (!node || node->length <= 0) {
    modulePopLoading();
    return valueNull();
  }

  RuntimeEnv *mod_env = semCreateEnv(NULL);
  if (!mod_env) {
    modulePopLoading();
    return valueNull();
  }
  stdlibInit(mod_env);
  builtinsInit(mod_env); /* len, type, … wajib tersedia di scope module */

  const char *prev_path = g_source_file_path;
  char *module_source_path = NULL;

  int root = 0;
  for (int i = 0; i < node->length; i++)
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }

  {
    if (module_path[0] == '/') {
      module_source_path = gcstrdup(module_path);
      g_source_file_path = module_source_path;
    } else {
      char *src_dir = dirName(prev_path);
      char *mod_full = NULL;
      if (hasDotSlash(module_path)) {
        mod_full = resolveDotPath(module_path, src_dir);
      } else {
        char *mod_file = resolveModulePath(module_path);
        if (mod_file) {
          mod_full = joinPath(src_dir, mod_file);
          free(mod_file);
        }
      }
      free(src_dir);
      if (mod_full) {
        module_source_path = mod_full;
        gcreg(module_source_path);
        g_source_file_path = module_source_path;
      }
    }
  }

  (void)interpretNode(node, root, mod_env, mod_error);
  propagateModuleErrors(error, mod_error);

  bool has_export = false;
  bool has_decl_export = false;
  {
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_MOD) continue;

      if (decl->mod.type == ExportDecl) {
        has_export = true;
        /* `export *` (ie.txt export #1): ekspor semua milik file ini —
         * hasil module = whole-env snapshot, bukan jalur deklaratif. */
        bool self_star = !decl->mod.source && decl->mod.entryCount == 1 &&
                         decl->mod.entries[0].type == MOD_WILD;
        if (!self_star && decl->mod.source) has_decl_export = true;
      } else if (decl->mod.type == NamespaceDecl) {
        has_export = true;
      }
    }
  }

  bool enforce = require_export || cross_boundary;
  if (!has_export && enforce) {
    /* Tree export (design/import_export.txt): leaf package tanpa export
     * sendiri tetap terjangkau VIA parent index-nya. `import z from
     * ./x.y.z` bukan akses langsung ke z.rp — loader mengalihkannya ke
     * `x/y/index.rp` dan menyerahkan ke entry import untuk mengambil
     * member `z` dari hasil export index cabang itu. Hanya leaf non-index
     * yang dialihkan; index tanpa export tetap ImportError (bounded —
     * rantai redirect berhenti di index pertama). */
    if (cross_boundary) {
      const char *slash = strrchr(canonical, '/');
      bool is_index = slash && strcmp(slash, "/index.rp") == 0;
      if (!is_index && slash && slash != canonical) {
        size_t dlen = (size_t)(slash - canonical);
        char *idx = malloc(dlen + 10);
        if (idx) {
          memcpy(idx, canonical, dlen);
          idx[dlen] = '\0';
          strcat(idx, "/index.rp");
          if (fileExists(idx)) {
            /* Cache leaf SEBELUM redirect: parent index pasti me-load
             * leaf yang sama untuk re-export — dengan cache, leaf hanya
             * dieksekusi sekali dan parent langsung pakai hasil ini. */
            moduleCachePut(gcstrdup(canonical), buildWholeEnvValue(mod_env));
            g_source_file_path = prev_path;
            modulePopLoading(); /* lepas frame leaf sebelum muat index */
            RuntimeValue parent_result = loadModuleFileError(idx, require_export, error);
            free(idx);
            return parent_result;
          }
          free(idx);
        }
      }
    }
    if (error) {
      static char message[MAX_MESSAGE_LENGTH];
      snprintf(message, sizeof(message),
               "Module '%s' does not export anything — nothing to import\n"
               "  note: add `export <name>` or `export <name> from ./<file>` in '%s'",
               canonical, canonical);
      setRuntimeErrorLocation(caller_line, caller_row); /* undo lokasi module dalam */
      addError(error, (ErrorInfo){.file = canonical,
                                  .code = "ImportError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR});
    }
    g_source_file_path = prev_path;
    modulePopLoading(); /* membebaskan canonical — pesan sudah disalin */
    return valueNull();
  }

  /* Hasil akhir module — diputuskan sekali di bawah, lalu di-cache.
   * Whole-env mencakup: module tanpa export yang diizinkan, namespace
   * tanpa re-export source, dan file yang hanya punya local `export x`
   * markers. */
  RuntimeValue mod_result = valueNull();
  if (!has_decl_export) {
    mod_result = buildWholeEnvValue(mod_env); /* prev_path/pop/cache di tail */
  }

  if (has_decl_export) {
    struct RuntimeObjectEntry *entries = NULL;
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_MOD || decl->mod.type != ExportDecl) continue;
      struct AstMod *mod = &decl->mod;

      /* Bare re-export (design/import_export.txt §Bare): `export test`
       * di index — muat ./test.rp (atau ./test/index.rp) sejajar dan
       * terbitkan sebagai member bernama `test`. Bukan file sejajar?
       * Bukan payload — member tetap mengalir lewat whole-env. */
      if (!mod->source) {
        /* `export *` self: tidak menerbitkan apa pun di sini — whole-env
         * snapshot (jalur !has_decl_export) yang memuat semuanya. */
        if (mod->entryCount == 1 && mod->entries[0].type == MOD_WILD) continue;
        for (int i = 0; i < mod->entryCount; i++) {
          AstModEntry *en = &mod->entries[i];
          if (!en->name || en->type != MOD_ID) continue;
          char barePath[64];
          snprintf(barePath, sizeof(barePath), "./%s", en->name);
          RuntimeValue bare_val = loadModuleFileError(barePath, false, error);
          if (bare_val.type != VALUE_OBJECT) continue;
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(en->name);
          se->value = bare_val;
          se->next = entries;
          entries = se;
        }
        continue;
      }

      RuntimeValue mod_val = loadModuleFileError(mod->source, false, error);
      if (mod_val.type != VALUE_OBJECT) continue;

      /* Self-entry `.` (`export . -> { ... }`): bukan re-export binding —
       * hanya marker module itu sendiri; tidak ada payload untuk
       * diterbitkan ke consumer. */
      if (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].name &&
          strcmp(mod->entries[0].name, ".") == 0)
        continue;

      /* Leading star re-export: `export * from ./X` — terbitkan seluruh
       * member source (bentuk kanonik design/import_export.txt). Policy
       * private tetap ditegakkan; member private diterbitkan sebagai null
       * + _private metadata, konsisten dengan re-export namespace. */
      if (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_WILD &&
          mod->entries[0].name && mod->entries[0].name[0] == '\0') {
        for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries; fe; fe = fe->next) {
          bool is_private = false;
          for (int pi = 0; pi < mod->policyCount; pi++) {
            if (mod->policies[pi].name && strcmp(fe->key, mod->policies[pi].name) == 0 &&
                mod->policies[pi].value && strcmp(mod->policies[pi].value, "private") == 0) {
              is_private = true;
              break;
            }
          }
          if (is_private) {
            struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
            pe->key = gcstrdup(fe->key);
            pe->value = valueNull();
            pe->next = entries;
            entries = pe;
            continue;
          }
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(fe->key);
          se->value = fe->value;
          se->next = entries;
          entries = se;
        }
        struct RuntimeObjectEntry *priv_entries = NULL;
        for (int pi = 0; pi < mod->policyCount; pi++) {
          if (mod->policies[pi].name && mod->policies[pi].value &&
              strcmp(mod->policies[pi].value, "private") == 0) {
            struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
            pe->key = gcstrdup(mod->policies[pi].name);
            pe->value = valueNull();
            pe->next = priv_entries;
            priv_entries = pe;
          }
        }
        if (priv_entries) {
          struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
          pe->key = gcstrdup("_private");
          pe->value = valueObject(priv_entries);
          pe->next = entries;
          entries = pe;
        }
        continue;
      }

      /* Namespace re-export: single plain entry. Member-first: binding
       * bernama sama di module sumber (mis. fungsi `open` di open.rp)
       * menang atas whole module — konsisten dengan computeExportBindings. */
      if (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_ID &&
          mod->entries[0].name) {
        const char *ns_name = mod->entries[0].name;

        RuntimeValue member_val;
        if (!mod->policies && valueObjectGet(mod_val, ns_name, &member_val)) {
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(ns_name);
          se->value = member_val;
          se->next = entries;
          entries = se;
          continue;
        }

        if (mod->policyCount > 0 && mod->policies) {
          struct RuntimeObjectEntry *filtered = NULL;
          for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries; fe; fe = fe->next) {
            bool is_private = false;
            for (int pi = 0; pi < mod->policyCount; pi++) {
              if (mod->policies[pi].name && strcmp(fe->key, mod->policies[pi].name) == 0 &&
                  mod->policies[pi].value && strcmp(mod->policies[pi].value, "private") == 0) {
                is_private = true;
                break;
              }
            }
            if (!is_private) {
              struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
              se->key = gcstrdup(fe->key);
              se->value = fe->value;
              se->next = filtered;
              filtered = se;
            }
          }
          struct RuntimeObjectEntry *priv_entries = NULL;
          for (int pi = 0; pi < mod->policyCount; pi++) {
            if (mod->policies[pi].name && mod->policies[pi].value &&
                strcmp(mod->policies[pi].value, "private") == 0) {
              struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
              pe->key = gcstrdup(mod->policies[pi].name);
              pe->value = valueNull();
              pe->next = priv_entries;
              priv_entries = pe;
            }
          }
          if (priv_entries) {
            struct RuntimeObjectEntry *pe = gccalloc(1, sizeof(*pe));
            pe->key = gcstrdup("_private");
            pe->value = valueObject(priv_entries);
            pe->next = filtered;
            filtered = pe;
          }
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(ns_name);
          se->value = valueObject(filtered);
          se->next = entries;
          entries = se;
        } else {
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(ns_name);
          se->value = mod_val;
          se->next = entries;
          entries = se;
        }
        continue;
      }

      /* Selective: bind each named member. Guard !en->name dihapus:
       * tanpa member chain, entry selective selalu bernama. */
      for (int i = 0; i < mod->entryCount; i++) {
        AstModEntry *en = &mod->entries[i];
        RuntimeValue item_val;
        if (en->name && valueObjectGet(mod_val, en->name, &item_val)) {
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(en->key ? en->key : en->name);
          se->value = item_val;
          se->next = entries;
          entries = se;
        }
      }
    }
    mod_result = valueObject(entries); /* prev_path/pop/cache di tail */
  }

  g_source_file_path = prev_path;
  /* Salinan sendiri untuk cache — frame di-pop membebaskan canonical. */
  moduleCachePut(gcstrdup(canonical), mod_result);
  modulePopLoading();
  return mod_result;
}
