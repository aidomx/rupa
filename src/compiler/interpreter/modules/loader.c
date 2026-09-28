#include <rupa.h>
/* setelah rupa.h: guard-nya bergantung RUPA_PACKAGE_H */
#include "module.h"
/* Module Loader — entry: muat & eksekusi file .rp sebagai modul.
 * The interpreter dispatcher (interpretNode) is in dispatch.c.
 *
 * Unit hasil split (rules.md: modular, jangan membengkak):
 *   loader_state.c   — state global, guard circular import, module cache
 *   loader_path.c    — utilitas & resolusi path import (relatif/absolut)
 *   loader_export.c  — pembangunan object export entries
 *   loader.c         — entry loadModuleFile/loadModuleFileError (file ini)
 */

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
    full_path = resolveAbsModulePath(module_path);
  } else {
    char *source_dir = modDirName(g_source_file_path);
    if (hasDotSlash(module_path)) {
      full_path = resolveDotPath(module_path, source_dir);
    } else if (strchr(module_path, '/') && strlen(module_path) > 3 &&
               strcmp(module_path + strlen(module_path) - 3, ".rp") == 0) {
      if (strncmp(module_path, "stdlib/", 7) == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
          full_path = modJoinPath(cwd, module_path);
        } else {
          full_path = strdup(module_path);
        }
      } else {
        full_path = modJoinPath(source_dir, module_path);
      }
    } else {
      char *rel_path = modResolveModulePath(module_path);
      if (rel_path) {
        full_path = modJoinPath(source_dir, rel_path);
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
      char *abs_resolved = resolveAbsModulePath(module_path);
      module_source_path = gcstrdup(abs_resolved ? abs_resolved : module_path);
      free(abs_resolved);
      g_source_file_path = module_source_path;
    } else {
      char *src_dir = modDirName(prev_path);
      char *mod_full = NULL;
      if (hasDotSlash(module_path)) {
        mod_full = resolveDotPath(module_path, src_dir);
      } else {
        char *mod_file = modResolveModulePath(module_path);
        if (mod_file) {
          mod_full = modJoinPath(src_dir, mod_file);
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
  const char *ns_bare = NULL; /* nama namespace file-level, bila ada */
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
        /* Namespace file-level (design namespace): `namespace user` tanpa
         * `{ }` — export surface file dibungkus nama namespace ini. */
        if (decl->mod.bare && decl->mod.source) ns_bare = decl->mod.source;
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
          if (modFileExists(idx)) {
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
    mod_result = modBuildExportEntries(node, root, module_path, error); /* tail di bawah */
  }

  g_source_file_path = prev_path;

  /* Namespace file-level (design namespace): bungkus surface dalam satu
   * object bernama namespace — `import user from ./b` lalu `user.x`.
   * Member private (dari mode pub) tetap terlihat sebagai null +
   * `_private` di dalam namespace → akses = PrivateError. */
  if (ns_bare) {
    struct RuntimeObjectEntry *wrap = gccalloc(1, sizeof(*wrap));
    wrap->key = gcstrdup(ns_bare);
    wrap->value = mod_result;
    wrap->next = NULL;
    mod_result = valueObject(wrap);
  }

  /* Salinan sendiri untuk cache — frame di-pop membebaskan canonical. */
  moduleCachePut(gcstrdup(canonical), mod_result);
  modulePopLoading();
  return mod_result;
}
