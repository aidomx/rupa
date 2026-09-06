#include <rupa.h>

/* Global event loop for async operations. Single-threaded interpreter. */
static struct EventLoop *g_event_loop = NULL;

/* Global source file path for resolving relative imports. */
static const char *g_source_file_path = NULL;

void setSourceFilePath(const char *path) { g_source_file_path = path; }
const char *getSourceFilePath(void) { return g_source_file_path; }

struct EventLoop *getEventLoop(void) { return g_event_loop; }

/*
 * Resolve a dotted module path to a file path.
 * E.g. "modules.test_1" -> "modules/test_1.rp"
 * Caller must free() the result.
 */
static bool fileExists(const char *path) {
  if (!path) return false;
  FILE *f = fopen(path, "rb");
  if (f) { fclose(f); return true; }
  return false;
}

static char *resolveModulePath(const char *module_path) {
  if (!module_path) return NULL;
  /* Strip ./ prefix */
  const char *path = module_path;
  if (path[0] == '.' && path[1] == '/') path += 2;
  int len = (int)strlen(path);
  char *buf = malloc(len + 4); /* +4 for .rp + null */
  if (!buf) return NULL;
  for (int i = 0; i < len; i++)
    buf[i] = path[i] == '.' ? '/' : path[i];
  buf[len] = '\0';
  strcat(buf, ".rp");
  return buf;
}

/*
 * Build a full file path by joining a directory and a relative path.
 * E.g. dir="/home/user/project/tests/syntax", rel="modules/test_1.rp"
 * -> "/home/user/project/tests/syntax/modules/test_1.rp"
 * Caller must free() the result.
 */
static char *joinPath(const char *dir, const char *rel) {
  if (!dir || !rel) return rel ? strdup(rel) : NULL;
  int dlen = (int)strlen(dir);
  int rlen = (int)strlen(rel);
  /* Ensure dir ends with / */
  int need_sep = (dlen > 0 && dir[dlen - 1] != '/');
  char *buf = malloc(dlen + need_sep + rlen + 1);
  if (!buf) return NULL;
  memcpy(buf, dir, dlen);
  if (need_sep) buf[dlen] = '/';
  memcpy(buf + dlen + need_sep, rel, rlen + 1);
  return buf;
}

/*
 * Get the directory portion of a file path.
 * Caller must free() the result.
 */
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

/* Check if path has ./ or ../ prefix */
static bool hasDotSlash(const char *path) {
  if (!path || path[0] != '.') return false;
  if (path[1] == '/') return true;
  if (path[1] == '.' && path[2] == '/') return true;
  return false;
}

/* Resolve a ./ or ../ path: try directory/index.rp first, then file.rp */
static char *resolveDotPath(const char *module_path, const char *source_dir) {
  if (!module_path || !source_dir) return NULL;

  /* Build the relative path: convert dots (except ..) to slashes */
  int mlen = (int)strlen(module_path);
  char *rel = malloc(mlen + 16); /* room for /index.rp or .rp */
  if (!rel) return NULL;

  int j = 0;
  for (int i = 0; i < mlen; i++) {
    /* Preserve .. as-is */
    if (module_path[i] == '.' && i + 1 < mlen && module_path[i + 1] == '.') {
      rel[j++] = '.';
      rel[j++] = '.';
      i++; /* skip second dot */
    } else if (module_path[i] == '.') {
      rel[j++] = '/';
    } else {
      rel[j++] = module_path[i];
    }
  }
  rel[j] = '\0';

  /* Try as file: rel.rp */
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

  /* Try as directory: rel/index.rp */
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

/*
 * Load and execute a local .rp file as a module.
 * Returns a RuntimeValue of type VALUE_OBJECT containing exported symbols,
 * or valueNull() on failure.
 */
static RuntimeValue loadModuleFile(const char *module_path, bool require_export) {
  if (!module_path) return valueNull();

  char *full_path = NULL;

  /* Absolute path → use directly */
  if (module_path[0] == '/') {
    full_path = strdup(module_path);
  } else {
    char *source_dir = dirName(g_source_file_path);
    if (hasDotSlash(module_path)) {
      /* ./path → try file, then directory/index.rp */
      full_path = resolveDotPath(module_path, source_dir);
    } else if (strchr(module_path, '/') && strlen(module_path) > 3 &&
               strcmp(module_path + strlen(module_path) - 3, ".rp") == 0) {
      /* Already a file path with .rp extension */
      if (strncmp(module_path, "stdlib/", 7) == 0) {
        /* stdlib paths are relative to CWD (project root) */
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
      /* Dotted path: modules.a → modules/a.rp */
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

  /* Create a State and read the file */
  State *state = createGlobalState(10, false);
  if (!state || !state->repl || !state->repl->buffer) {
    free(full_path);
    return valueNull();
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->repl->buffer;
  if (!readfile(full_path, buffer)) {
    free(full_path);
    return valueNull();
  }
  free(full_path);

  /* Lex and parse */
  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0)
    return valueNull();

  /* Generate AST */
  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);
  Error *error = createError(10);

  if (!node || node->length <= 0)
    return valueNull();

  /* Create a new environment for the module */
  RuntimeEnv *mod_env = semCreateEnv(NULL);
  if (!mod_env)
    return valueNull();
  stdlibInit(mod_env);

  /* Save and restore source file path for nested imports */
  const char *prev_path = g_source_file_path;

  int root = 0;
  for (int i = 0; i < node->length; i++)
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }

  /* Set source path to the loaded file for nested imports */
  {
    if (module_path[0] == '/') {
      /* Absolute path → use as-is */
      g_source_file_path = strdup(module_path);
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
      if (mod_full)
        g_source_file_path = mod_full; /* leaked intentionally for GC */
    }
  }

  /* Execute the module */
  (void)interpretNode(node, root, mod_env, error);

  /* Check if module has export statement. If not, nothing is exported.
   * Two types:
   *   NODE_EXPORT       → export everything user-defined from mod_env
   *   NODE_EXPORT_DECL  → build export directly from AST (load source modules)
   */
  bool has_export = false;
  bool has_decl_export = false;
  {
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId >= 0 && d->nodeId < node->length) {
        AstNode *decl = &node->ast[d->nodeId];
        if (decl->type == NODE_EXPORT) {
          has_export = true;
          break;
        }
        if (decl->type == NODE_EXPORT_DECL) {
          has_export = true;
          has_decl_export = true;
          break;
        }
      }
    }
  }

  if (!has_export && require_export) {
    g_source_file_path = prev_path;
    return valueNull();
  }

  /* If no export but require_export is false, return all user-defined bindings */
  if (!has_export && !require_export) {
    struct RuntimeObjectEntry *entries = NULL;
    for (RuntimeBinding *b = mod_env->bindings; b; b = b->next) {
      if (b->value.type == VALUE_OBJECT) continue;
      if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
      struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
      e->key = strdup(b->name);
      e->value = b->value;
      e->next = entries;
      entries = e;
    }
    g_source_file_path = prev_path;
    return resultNormal(valueObject(entries)).value;
  }

  /* NODE_EXPORT_DECL: build export object directly from AST info.
   * For each export decl node, load the source module and build
   * the result according to selective/namespace/policy rules. */
  if (has_decl_export) {
    struct RuntimeObjectEntry *entries = NULL;
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_EXPORT_DECL) continue;
      struct AstExport *exp = &decl->astExport;

      /* Resolve source path */
      const char *src_path = NULL;
      if (exp->sourcePath >= 0 && exp->sourcePath < node->length) {
        AstNode *srcAst = &node->ast[exp->sourcePath];
        if (srcAst->type == NODE_LITERAL_ID) src_path = srcAst->string.value;
        else if (srcAst->type == NODE_IDENTIFIER) src_path = srcAst->identifier.name;
      }
      if (!src_path) continue;

      /* Load source module (require_export=false: source doesn't need its own export) */
      RuntimeValue mod_val = loadModuleFile(src_path, false);
      if (mod_val.type != VALUE_OBJECT) continue;

      /* Namespace export: export c from ./c -> { a: private }
       * Bind the whole source module under namespace name */
      if (exp->namespaceName >= 0 && exp->namespaceName < node->length) {
        const char *ns_name = NULL;
        AstNode *nsAst = &node->ast[exp->namespaceName];
        if (nsAst->type == NODE_LITERAL_ID) ns_name = nsAst->string.value;
        else if (nsAst->type == NODE_IDENTIFIER) ns_name = nsAst->identifier.name;
        if (!ns_name) continue;

        /* Apply policies if present */
        if (exp->policyCount > 0 && exp->policies) {
          struct RuntimeObjectEntry *filtered = NULL;
          for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries;
               fe; fe = fe->next) {
            bool is_private = false;
            for (int pi = 0; pi < exp->policyCount; pi++) {
              if (exp->policies[pi].nameNode >= 0 &&
                  exp->policies[pi].nameNode < node->length) {
                AstNode *polAst = &node->ast[exp->policies[pi].nameNode];
                const char *pol_name = NULL;
                if (polAst->type == NODE_LITERAL_ID) pol_name = polAst->string.value;
                else if (polAst->type == NODE_IDENTIFIER) pol_name = polAst->identifier.name;
                if (pol_name && strcmp(fe->key, pol_name) == 0 &&
                    exp->policies[pi].policy &&
                    strcmp(exp->policies[pi].policy, "private") == 0) {
                  is_private = true;
                  break;
                }
              }
            }
            if (!is_private) {
              struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
              se->key = strdup(fe->key);
              se->value = fe->value;
              se->next = filtered;
              filtered = se;
            }
          }
          /* Add _private entry: object with private name keys */
          struct RuntimeObjectEntry *priv_entries = NULL;
          for (int pi = 0; pi < exp->policyCount; pi++) {
            if (exp->policies[pi].nameNode >= 0 &&
                exp->policies[pi].nameNode < node->length &&
                exp->policies[pi].policy &&
                strcmp(exp->policies[pi].policy, "private") == 0) {
              AstNode *polAst = &node->ast[exp->policies[pi].nameNode];
              const char *pol_name = NULL;
              if (polAst->type == NODE_LITERAL_ID) pol_name = polAst->string.value;
              else if (polAst->type == NODE_IDENTIFIER) pol_name = polAst->identifier.name;
              if (pol_name) {
                struct RuntimeObjectEntry *pe = calloc(1, sizeof(*pe));
                pe->key = strdup(pol_name);
                pe->value = valueNull();
                pe->next = priv_entries;
                priv_entries = pe;
              }
            }
          }
          if (priv_entries) {
            struct RuntimeObjectEntry *pe = calloc(1, sizeof(*pe));
            pe->key = strdup("_private");
            pe->value = valueObject(priv_entries);
            pe->next = filtered;
            filtered = pe;
          }
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(ns_name);
          se->value = valueObject(filtered);
          se->next = entries;
          entries = se;
        } else {
          /* No policies — bind whole module object */
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(ns_name);
          se->value = mod_val;
          se->next = entries;
          entries = se;
        }
      }
      /* Selective export: export a, b from ./c
       * Pick named items from source module */
      else if (exp->selectiveItems >= 0 && exp->selectiveItems < node->length) {
        AstNode *items_node = &node->ast[exp->selectiveItems];
        if (items_node->type == NODE_ARRAY) {
          for (int i = 0; i < items_node->array.length; i++) {
            int nid = items_node->array.elements[i];
            if (nid < 0 || nid >= node->length) continue;
            AstNode *item = &node->ast[nid];
            const char *item_name = NULL;
            if (item->type == NODE_LITERAL_ID) item_name = item->string.value;
            else if (item->type == NODE_IDENTIFIER) item_name = item->identifier.name;
            if (!item_name) continue;
            RuntimeValue item_val;
            if (valueObjectGet(mod_val, item_name, &item_val)) {
              struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
              se->key = strdup(item_name);
              se->value = item_val;
              se->next = entries;
              entries = se;
            }
          }
        }
      }
    }
    g_source_file_path = prev_path;
    return resultNormal(valueObject(entries)).value;
  }

  /* NODE_EXPORT (legacy): collect all user-defined bindings from mod_env.
   * Skip stdlib builtins (functions + modules like math, os, json). */
  struct RuntimeObjectEntry *entries = NULL;
  for (RuntimeBinding *b = mod_env->bindings; b; b = b->next) {
    /* Skip stdlib VALUE_OBJECT modules (math, os, json, etc.) */
    if (b->value.type == VALUE_OBJECT) continue;
    /* Skip stdlib native functions (print, type, len, etc.) */
    if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
    struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
    e->key = strdup(b->name);
    e->value = b->value;
    e->next = entries;
    entries = e;
  }

  g_source_file_path = prev_path;
  return resultNormal(valueObject(entries)).value;
}

InterpreterResult interpretNode(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length)
    return resultNormal(valueNull());

  if (n->ast[id].type == NODE_PROGRAM) {
    RuntimeValue last = valueNull();
    for (AstDeclaration *d = n->ast[id].program.declarations; d; d = d->next) {
      InterpreterResult r = interpretNode(n, d->nodeId, e, x);
      last = r.value;
    }
    /* After all top-level statements, run the event loop to resolve
     * any pending async operations. */
    if (g_event_loop)
      eventLoopRun(n, g_event_loop, e, x);
    return resultNormal(last);
  }

  switch (n->ast[id].type) {
  case NODE_IMPORT: {
    /* Handle import statements:
     *   import os from rupa          -> bind stdlib module
     *   import X from rupa.Y         -> bind specific stdlib
     *   import X from ./path         -> local file/dir (./ bypasses stdlib)
     *   import X from path.to.mod    -> stdlib first, then local
     */
    int expr_id = n->ast[id].module.value;
    int name_id = n->ast[id].module.name;

    if (name_id >= 0 && expr_id >= 0 && expr_id < n->length) {
      AstNode *mod_ast = &n->ast[name_id];
      const char *mod_name = NULL;
      if (mod_ast->type == NODE_LITERAL_ID) mod_name = mod_ast->string.value;
      else if (mod_ast->type == NODE_IDENTIFIER) mod_name = mod_ast->identifier.name;

      if (!mod_name)
        return resultNormal(valueNull());

      bool local_path = hasDotSlash(mod_name);
      RuntimeValue module_val;
      bool found = false;

      if (local_path) {
        /* ./ prefix → skip stdlib entirely, go to file/dir */
        module_val = loadModuleFile(mod_name, true);
        found = (module_val.type == VALUE_OBJECT);
        /* Sub-module fallback: import d from ./modules → ./modules.d → modules/d.rp */
        if (!found && n->ast[expr_id].type != NODE_ARRAY) {
          AstNode *tmp_ast = &n->ast[expr_id];
          const char *tmp_func = NULL;
          if (tmp_ast->type == NODE_LITERAL_ID) tmp_func = tmp_ast->string.value;
          else if (tmp_ast->type == NODE_IDENTIFIER) tmp_func = tmp_ast->identifier.name;
          if (tmp_func) {
            char combined[512];
            snprintf(combined, sizeof(combined), "%s.%s", mod_name, tmp_func);
            module_val = loadModuleFile(combined, true);
            if (module_val.type == VALUE_OBJECT) {
              found = true;
              semSet(e, tmp_func, module_val);
            }
          }
        }
      } else {
        /* `from rupa` is the root namespace for Rupa modules.
         * For `import math from rupa`, resolve the imported name (`math`)
         * as a package first. This lets a Rupa module replace a C module
         * without requiring the old `rupa.math` spelling. */
        if (strcmp(mod_name, "rupa") == 0 &&
            n->ast[expr_id].type != NODE_ARRAY) {
          AstNode *root_ast = &n->ast[expr_id];
          const char *root_name = NULL;
          if (root_ast->type == NODE_LITERAL_ID)
            root_name = root_ast->string.value;
          else if (root_ast->type == NODE_IDENTIFIER)
            root_name = root_ast->identifier.name;

          if (root_name) {
            const char *root_path = stdlibFindModule(root_name);
            if (root_path) {
              module_val = loadModuleFile(root_path, true);
              found = (module_val.type == VALUE_OBJECT);
            }
            /* C modules remain available through the same root when no
             * Rupa package shadows them (e.g. `import os from rupa`). */
            if (!found)
              found = stdlibGetModule(root_name, &module_val);
          }
        }

        /* No `rupa` package matched → retain the existing module resolution
         * for direct C modules, local modules, and external packages. */
        if (!found) {
          /* No ./ prefix → try stdlib first */
          found = stdlibGetModule(mod_name, &module_val);
          if (!found) {
            /* Try direct file load */
            module_val = loadModuleFile(mod_name, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
        /* If still not found, try sub-module:
         * import d from modules → try modules/d.rp */
        if (!found && n->ast[expr_id].type != NODE_ARRAY) {
          AstNode *tmp_ast = &n->ast[expr_id];
          const char *tmp_func = NULL;
          if (tmp_ast->type == NODE_LITERAL_ID) tmp_func = tmp_ast->string.value;
          else if (tmp_ast->type == NODE_IDENTIFIER) tmp_func = tmp_ast->identifier.name;
          if (tmp_func) {
            char combined[512];
            snprintf(combined, sizeof(combined), "%s.%s", mod_name, tmp_func);
            module_val = loadModuleFile(combined, true);
            if (module_val.type == VALUE_OBJECT) {
              found = true;
              semSet(e, tmp_func, module_val);
            }
          }
        }
        /* Try rupa.X → external stdlib package (e.g. rupa.stark → ~/.rupa/stdlib/stark/) */
        if (!found && strncmp(mod_name, "rupa.", 5) == 0) {
          const char *pkg_name = mod_name + 5;
          const char *ext_path = stdlibFindModule(pkg_name);
          if (ext_path) {
            module_val = loadModuleFile(ext_path, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
        if (!found) {
          /* Try external stdlib */
          const char *ext_path = stdlibFindModule(mod_name);
          if (ext_path) {
            module_val = loadModuleFile(ext_path, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
      }

      if (!found)
        return resultNormal(valueNull());

      /* `import package from rupa` binds the resolved package itself.
       * It is different from `import name from package`, which extracts a
       * member named `name` from an already resolved module. */
      bool rupa_root_import = strcmp(mod_name, "rupa") == 0;

      /* import X, Z from module (array of names) */
      if (n->ast[expr_id].type == NODE_ARRAY) {
        int len = n->ast[expr_id].array.length;
        for (int i = 0; i < len; i++) {
          int nid = n->ast[expr_id].array.elements[i];
          if (nid < 0 || nid >= n->length) continue;
          AstNode *na = &n->ast[nid];
          const char *fn = NULL;
          if (na->type == NODE_LITERAL_ID) fn = na->string.value;
          else if (na->type == NODE_IDENTIFIER) fn = na->identifier.name;
          if (!fn) continue;
          RuntimeValue fn_val;
          if (valueObjectGet(module_val, fn, &fn_val))
            semSet(e, fn, fn_val);
        }
      }
      /* import X from module (single name) */
      else {
        AstNode *func_ast = &n->ast[expr_id];
        const char *func_name = NULL;
        if (func_ast->type == NODE_LITERAL_ID)
          func_name = func_ast->string.value;
        else if (func_ast->type == NODE_IDENTIFIER)
          func_name = func_ast->identifier.name;
        if (func_name) {
          RuntimeValue fn_val;
          if (rupa_root_import) {
            /* `import math from rupa` → math is the package object. */
            semSet(e, func_name, module_val);
          } else if (valueObjectGet(module_val, func_name, &fn_val)) {
            /* Key found in module (e.g. import add from ./math.add) */
            semSet(e, func_name, fn_val);
          } else if (hasDotSlash(mod_name)) {
            /* ./path → bind whole module as alias */
            semSet(e, func_name, module_val);
          }
        }
      }
    } else if (expr_id >= 0 && expr_id < n->length) {
      /* import os from rupa -> look up module directly */
      AstNode *expr = &n->ast[expr_id];
      const char *module_name = NULL;
      if (expr->type == NODE_LITERAL_ID)
        module_name = expr->string.value;
      else if (expr->type == NODE_IDENTIFIER)
        module_name = expr->identifier.name;

      if (module_name) {
        RuntimeValue module_val;
        if (stdlibGetModule(module_name, &module_val)) {
          semSet(e, module_name, module_val);
        } else {
          const char *ext_path = stdlibFindModule(module_name);
          if (ext_path) {
            module_val = loadModuleFile(ext_path, true);
            if (module_val.type == VALUE_OBJECT)
              semSet(e, module_name, module_val);
          } else {
            module_val = loadModuleFile(module_name, true);
            if (module_val.type == VALUE_OBJECT)
              semSet(e, module_name, module_val);
          }
        }
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_MODULE_IMPORT: {
    /* Handle: import a.create, b.login as auth, d.* from modules as m
     * 1. Get base path
     * 2. For each entry, resolve path and load file/function
     * 3. Build namespace object
     * 4. Bind to alias (or base name)
     */
    AstNode *importNode = &n->ast[id];
    int basePathId = importNode->moduleImport.basePath;
    struct AstModuleImportEntry *entries = importNode->moduleImport.entries;
    int entryCount = importNode->moduleImport.entryCount;
    int aliasId = importNode->moduleImport.alias;

    if (basePathId < 0 || basePathId >= n->length)
      return resultNormal(valueNull());

    /* Get base path string */
    const char *base_path = NULL;
    AstNode *baseAst = &n->ast[basePathId];
    if (baseAst->type == NODE_LITERAL_ID) base_path = baseAst->string.value;
    else if (baseAst->type == NODE_IDENTIFIER) base_path = baseAst->identifier.name;
    if (!base_path) return resultNormal(valueNull());

    /* Build namespace name: alias or base_path */
    const char *ns_name = base_path;
    if (aliasId >= 0 && aliasId < n->length) {
      AstNode *aliasAst = &n->ast[aliasId];
      if (aliasAst->type == NODE_LITERAL_ID) ns_name = aliasAst->string.value;
      else if (aliasAst->type == NODE_IDENTIFIER) ns_name = aliasAst->identifier.name;
    }

    /* Process entries and build namespace object */
    struct RuntimeObjectEntry *nsEntries = NULL;
    for (int i = 0; i < entryCount; i++) {
      struct AstModuleImportEntry *e = &entries[i];

      /* Get path string */
      const char *path_str = NULL;
      if (e->pathNode >= 0 && e->pathNode < n->length) {
        AstNode *pathAst = &n->ast[e->pathNode];
        if (pathAst->type == NODE_LITERAL_ID) path_str = pathAst->string.value;
        else if (pathAst->type == NODE_IDENTIFIER) path_str = pathAst->identifier.name;
      }
      if (!path_str) continue;

      /* Get alias string */
      const char *alias_str = NULL;
      if (e->aliasNode >= 0 && e->aliasNode < n->length) {
        AstNode *aliasAst = &n->ast[e->aliasNode];
        if (aliasAst->type == NODE_LITERAL_ID) alias_str = aliasAst->string.value;
        else if (aliasAst->type == NODE_IDENTIFIER) alias_str = aliasAst->identifier.name;
      }

      /* Build file path: base_path/path.rp
       * path_str is like "a.create" -> file is base_path/a.rp, function is create
       * path_str is like "d" (wildcard) -> file is base_path/d.rp, all functions
       */
      char file_path[512];
      const char *dot = strchr(path_str, '.');
      if (dot && !e->isWildcard) {
        /* Specific function: a.create -> base_path/a.rp */
        int prefix_len = (int)(dot - path_str);
        snprintf(file_path, sizeof(file_path), "%s/%.*s", base_path, prefix_len,
                 path_str);
      } else {
        /* Wildcard or single name: d.* or d -> base_path/d.rp */
        snprintf(file_path, sizeof(file_path), "%s/%s", base_path, path_str);
      }

      /* Load the module file */
      RuntimeValue mod_val = loadModuleFile(file_path, true);
      if (mod_val.type != VALUE_OBJECT) continue;

      /* Determine the key name for this entry */
      const char *key_name = alias_str ? alias_str : path_str;
      if (dot && !e->isWildcard) {
        /* For "a.create", key is "create" (or alias) */
        key_name = alias_str ? alias_str : dot + 1;
      }

      if (e->isWildcard) {
        if (alias_str) {
          /* Per-entry alias: a.* as form -> form = module object */
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(alias_str);
          se->value = mod_val;
          se->next = nsEntries;
          nsEntries = se;
        } else {
          /* No alias: flatten all functions into namespace */
          for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries;
               fe; fe = fe->next) {
            struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
            se->key = strdup(fe->key);
            se->value = fe->value;
            se->next = nsEntries;
            nsEntries = se;
          }
        }
      } else {
        /* Specific function: b.login -> extract login from b */
        const char *func_name = dot ? dot + 1 : path_str;
        RuntimeValue fn_val;
        if (valueObjectGet(mod_val, func_name, &fn_val)) {
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(key_name);
          se->value = fn_val;
          se->next = nsEntries;
          nsEntries = se;
        }
      }
    }

    /* Bind namespace to environment */
    if (aliasId >= 0) {
      /* Has namespace alias (e.g. "as m"): bind all under ns */
      semSet(e, ns_name, valueObject(nsEntries));
    } else {
      /* No namespace alias: bind each entry at top level */
      struct RuntimeObjectEntry *se = nsEntries;
      while (se) {
        semSet(e, se->key, se->value);
        se = se->next;
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_EXPORT: {
    /* export math / export module — no-op, loadModuleFile collects all bindings */
    return resultNormal(valueNull());
  }
  case NODE_EXPORT_DECL: {
    /* export a, b from ./c / export c from ./c / export c from ./c -> { a: private }
     * Execute: load source module, bind to environment. */
    {
      AstNode *expNode = &n->ast[id];
      struct AstExport *exp = &expNode->astExport;

      /* Resolve source path */
      const char *src_path = NULL;
      if (exp->sourcePath >= 0 && exp->sourcePath < n->length) {
        AstNode *srcAst = &n->ast[exp->sourcePath];
        if (srcAst->type == NODE_LITERAL_ID) src_path = srcAst->string.value;
        else if (srcAst->type == NODE_IDENTIFIER) src_path = srcAst->identifier.name;
      }
      if (!src_path) return resultNormal(valueNull());

      /* Load the source module (require_export=false: source doesn't need its own export) */
      RuntimeValue mod_val = loadModuleFile(src_path, false);

      /* Namespace export: export c from ./c
       * Bind the whole module as 'c' in environment */
      if (exp->namespaceName >= 0 && exp->namespaceName < n->length) {
        const char *ns_name = NULL;
        AstNode *nsAst = &n->ast[exp->namespaceName];
        if (nsAst->type == NODE_LITERAL_ID) ns_name = nsAst->string.value;
        else if (nsAst->type == NODE_IDENTIFIER) ns_name = nsAst->identifier.name;
        if (ns_name && mod_val.type == VALUE_OBJECT) {
          /* Apply policies: filter out private items */
          if (exp->policyCount > 0 && exp->policies) {
            struct RuntimeObjectEntry *filtered = NULL;
            for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries;
                 fe; fe = fe->next) {
              bool is_private = false;
              for (int pi = 0; pi < exp->policyCount; pi++) {
                if (exp->policies[pi].nameNode >= 0 &&
                    exp->policies[pi].nameNode < n->length) {
                  AstNode *polAst = &n->ast[exp->policies[pi].nameNode];
                  const char *pol_name = NULL;
                  if (polAst->type == NODE_LITERAL_ID) pol_name = polAst->string.value;
                  else if (polAst->type == NODE_IDENTIFIER) pol_name = polAst->identifier.name;
                  if (pol_name && strcmp(fe->key, pol_name) == 0 &&
                      exp->policies[pi].policy &&
                      strcmp(exp->policies[pi].policy, "private") == 0) {
                    is_private = true;
                    break;
                  }
                }
              }
              if (!is_private) {
                struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
                se->key = strdup(fe->key);
                se->value = fe->value;
                se->next = filtered;
                filtered = se;
              }
            }
            semSet(e, ns_name, valueObject(filtered));
          } else {
            semSet(e, ns_name, mod_val);
          }
        }
      }
      /* Selective export: export a, b from ./c
       * Bind specific items from source module */
      else if (exp->selectiveItems >= 0 && exp->selectiveItems < n->length) {
        AstNode *items_node = &n->ast[exp->selectiveItems];
        if (items_node->type == NODE_ARRAY && mod_val.type == VALUE_OBJECT) {
          for (int i = 0; i < items_node->array.length; i++) {
            int nid = items_node->array.elements[i];
            if (nid < 0 || nid >= n->length) continue;
            AstNode *item = &n->ast[nid];
            const char *item_name = NULL;
            if (item->type == NODE_LITERAL_ID) item_name = item->string.value;
            else if (item->type == NODE_IDENTIFIER) item_name = item->identifier.name;
            if (!item_name) continue;
            RuntimeValue item_val;
            if (valueObjectGet(mod_val, item_name, &item_val))
              semSet(e, item_name, item_val);
          }
        }
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_ASSIGN:
  case NODE_CONDITIONAL_ASSIGN:
  case NODE_ANNOTATION:
  case NODE_PRINT:
  case NODE_RETURN:
  case NODE_BLOCK:
  case NODE_IF:
  case NODE_BREAK:
  case NODE_CONTINUE:
  case NODE_FUNCTION_DECL:
  case NODE_LOOP:
  case NODE_CASE:
  case NODE_STRUCT_DECL:
  case NODE_MEMBER_ASSIGN:
    return interpretStatement(n, id, e, x);
  default:
    return interpretExpression(n, id, e, x);
  }
}

void interpreter(Node *node, Error *error) {
  if (!node || node->length <= 0)
    return;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env)
    return;

  /* Initialize standard library modules */
  stdlibInit(env);

  g_event_loop = eventLoopCreate();

  int root = 0;
  for (int i = 0; i < node->length; i++)
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  InterpreterResult result = interpretNode(node, root, env, error);
  if (result.flow == FLOW_ERROR || (error && error->size > 0))
    printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;

  /* Cleanup extracted stdlib */
  stdlibLoaderCleanup();
}
