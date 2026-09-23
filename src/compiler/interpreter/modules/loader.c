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

static char *resolveDotPath(const char *module_path, const char *source_dir) {
  if (!module_path || !source_dir) return NULL;

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

/* ---- Public API ---- */

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
        size_t plen = strlen(pkg_root);
        bool inside = strncmp(caller_path, pkg_root, plen) == 0 &&
                      (caller_path[plen] == '/' || caller_path[plen] == '\0');
        cross_boundary = !inside;
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
  bool has_namespace = false;
  {
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_MOD) continue;

      if (decl->mod.type == ExportDecl) {
        has_export = true;
        if (decl->mod.source) has_decl_export = true;
      } else if (decl->mod.type == NamespaceDecl) {
        has_export = true;
        has_namespace = true;
      }
    }
  }

  bool enforce = require_export || cross_boundary;
  if (!has_export && enforce) {
    g_source_file_path = prev_path;
    modulePopLoading();
    return valueNull();
  }

  if ((!has_export && !enforce) || (has_namespace && !has_decl_export)) {
    struct RuntimeObjectEntry *entries = NULL;
    for (RuntimeBinding *b = mod_env->bindings; b; b = b->next) {
      /* Runtime-only async status constants are implementation details,
       * never module exports. Objects must remain here because namespaces
       * are represented as RuntimeValue objects. */
      if (strcmp(b->name, "AWAIT") == 0 || strcmp(b->name, "SUCCESS") == 0 ||
          strcmp(b->name, "ERROR") == 0)
        continue;
      if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
      struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
      e->key = gcstrdup(b->name);
      e->value = b->value;
      e->next = entries;
      entries = e;
    }
    g_source_file_path = prev_path;
    modulePopLoading();
    return resultNormal(valueObject(entries)).value;
  }

  if (has_decl_export) {
    struct RuntimeObjectEntry *entries = NULL;
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_MOD || decl->mod.type != ExportDecl) continue;
      struct AstMod *mod = &decl->mod;
      if (!mod->source) continue; /* local export: no re-export payload */

      RuntimeValue mod_val = loadModuleFileError(mod->source, false, error);
      if (mod_val.type != VALUE_OBJECT) continue;

      /* Namespace re-export: single plain entry */
      if (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_ID &&
          mod->entries[0].name) {
        const char *ns_name = mod->entries[0].name;

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

      /* Selective: bind each named member */
      for (int i = 0; i < mod->entryCount; i++) {
        AstModEntry *en = &mod->entries[i];
        if (!en->name) continue;
        RuntimeValue item_val;
        if (valueObjectGet(mod_val, en->name, &item_val)) {
          struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
          se->key = gcstrdup(en->key ? en->key : en->name);
          se->value = item_val;
          se->next = entries;
          entries = se;
        }
      }
    }
    g_source_file_path = prev_path;
    modulePopLoading();
    return resultNormal(valueObject(entries)).value;
  }

  struct RuntimeObjectEntry *entries = NULL;
  for (RuntimeBinding *b = mod_env->bindings; b; b = b->next) {
    if (strcmp(b->name, "AWAIT") == 0 || strcmp(b->name, "SUCCESS") == 0 ||
        strcmp(b->name, "ERROR") == 0)
      continue;
    if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
    struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
    e->key = gcstrdup(b->name);
    e->value = b->value;
    e->next = entries;
    entries = e;
  }

  g_source_file_path = prev_path;
  modulePopLoading();
  return resultNormal(valueObject(entries)).value;
}
