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

/*
 * Load and execute a local .rp file as a module.
 * Returns a RuntimeValue of type VALUE_OBJECT containing exported symbols,
 * or valueNull() on failure.
 */
static RuntimeValue loadModuleFile(const char *module_path) {
  if (!module_path) return valueNull();

  /* Resolve dotted path to file path */
  char *rel_path = resolveModulePath(module_path);
  if (!rel_path) return valueNull();

  /* Build full path relative to current source file */
  char *source_dir = dirName(g_source_file_path);
  char *full_path = joinPath(source_dir, rel_path);

  free(rel_path);
  free(source_dir);

  if (!full_path) return valueNull();

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
  /* We use the full_path we already freed, so reconstruct from module_path */
  {
    char *mod_file = resolveModulePath(module_path);
    char *src_dir = dirName(prev_path);
    char *mod_full = joinPath(src_dir, mod_file);
    free(mod_file);
    free(src_dir);
    g_source_file_path = mod_full; /* leaked intentionally for GC */
  }

  /* Execute the module */
  (void)interpretNode(node, root, mod_env, error);

  /* Collect all function bindings from the module environment */
  struct RuntimeObjectEntry *entries = NULL;
  for (RuntimeBinding *b = mod_env->bindings; b; b = b->next) {
    if (b->value.type == VALUE_FUNCTION || b->value.type == VALUE_NATIVE_FUNCTION) {
      struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
      e->key = strdup(b->name);
      e->value = b->value;
      e->next = entries;
      entries = e;
    }
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
     *   import os from rupa        -> value=os(LITERAL_ID), name=-1
     *   import X from rupa.Y       -> value=X(LITERAL_ID),  name=Y(LITERAL_ID)
     *   import X, Z from rupa.Y    -> value=ARRAY,          name=Y(LITERAL_ID)
     *   import X from path.to.mod  -> local file import
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

      /* First try stdlib modules */
      RuntimeValue module_val;
      bool is_stdlib = stdlibGetModule(mod_name, &module_val);

      /* If not stdlib, check if direct file exists before loading */
      if (!is_stdlib) {
        char *direct_path = resolveModulePath(mod_name);
        if (direct_path) {
          char *source_dir = dirName(g_source_file_path);
          char *full = joinPath(source_dir, direct_path);
          free(direct_path);
          if (full && fileExists(full)) {
            free(full);
            module_val = loadModuleFile(mod_name);
            if (module_val.type == VALUE_OBJECT)
              is_stdlib = true;
          } else {
            free(full);
          }
        }
      }

      /* If module file not found and single import, try sub-module:
       * import d from modules -> try modules/d.rp */
      if (!is_stdlib && n->ast[expr_id].type != NODE_ARRAY) {
        AstNode *tmp_ast = &n->ast[expr_id];
        const char *tmp_func = NULL;
        if (tmp_ast->type == NODE_LITERAL_ID) tmp_func = tmp_ast->string.value;
        else if (tmp_ast->type == NODE_IDENTIFIER) tmp_func = tmp_ast->identifier.name;
        if (tmp_func) {
          char combined[512];
          snprintf(combined, sizeof(combined), "%s.%s", mod_name, tmp_func);
          module_val = loadModuleFile(combined);
          if (module_val.type == VALUE_OBJECT) {
            is_stdlib = true;
            semSet(e, tmp_func, module_val);
          }
        }
      }

      if (!is_stdlib)
        return resultNormal(valueNull());

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
          if (valueObjectGet(module_val, func_name, &fn_val))
            semSet(e, func_name, fn_val);
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
          /* Try local file import */
          module_val = loadModuleFile(module_name);
          if (module_val.type == VALUE_OBJECT)
            semSet(e, module_name, module_val);
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
      RuntimeValue mod_val = loadModuleFile(file_path);
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
}
