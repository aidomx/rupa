#include <rupa.h>
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

/* ---- Public API ---- */

RuntimeValue loadModuleFile(const char *module_path, bool require_export) {
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

  State *state = createGlobalState(10, false);
  if (!state || !state->buffer) {
    free(full_path);
    return valueNull();
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  if (!readfile(full_path, buffer)) {
    free(full_path);
    return valueNull();
  }
  free(full_path);

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0) return valueNull();

  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);
  Error *error = createError(10);

  if (!node || node->length <= 0) return valueNull();

  RuntimeEnv *mod_env = semCreateEnv(NULL);
  if (!mod_env) return valueNull();
  stdlibInit(mod_env);

  const char *prev_path = g_source_file_path;

  int root = 0;
  for (int i = 0; i < node->length; i++)
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }

  {
    if (module_path[0] == '/') {
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
      if (mod_full) g_source_file_path = mod_full;
    }
  }

  (void)interpretNode(node, root, mod_env, error);

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

  if (has_decl_export) {
    struct RuntimeObjectEntry *entries = NULL;
    AstNode *prog = &node->ast[root];
    for (AstDeclaration *d = prog->program.declarations; d; d = d->next) {
      if (d->nodeId < 0 || d->nodeId >= node->length) continue;
      AstNode *decl = &node->ast[d->nodeId];
      if (decl->type != NODE_EXPORT_DECL) continue;
      struct AstExport *exp = &decl->astExport;

      const char *src_path = NULL;
      if (exp->sourcePath >= 0 && exp->sourcePath < node->length) {
        AstNode *srcAst = &node->ast[exp->sourcePath];
        if (srcAst->type == NODE_LITERAL_ID)
          src_path = srcAst->string.value;
        else if (srcAst->type == NODE_IDENTIFIER)
          src_path = srcAst->identifier.name;
      }
      if (!src_path) continue;

      RuntimeValue mod_val = loadModuleFile(src_path, false);
      if (mod_val.type != VALUE_OBJECT) continue;

      if (exp->namespaceName >= 0 && exp->namespaceName < node->length) {
        const char *ns_name = NULL;
        AstNode *nsAst = &node->ast[exp->namespaceName];
        if (nsAst->type == NODE_LITERAL_ID)
          ns_name = nsAst->string.value;
        else if (nsAst->type == NODE_IDENTIFIER)
          ns_name = nsAst->identifier.name;
        if (!ns_name) continue;

        if (exp->policyCount > 0 && exp->policies) {
          struct RuntimeObjectEntry *filtered = NULL;
          for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries; fe; fe = fe->next) {
            bool is_private = false;
            for (int pi = 0; pi < exp->policyCount; pi++) {
              if (exp->policies[pi].nameNode >= 0 && exp->policies[pi].nameNode < node->length) {
                AstNode *polAst = &node->ast[exp->policies[pi].nameNode];
                const char *pol_name = NULL;
                if (polAst->type == NODE_LITERAL_ID)
                  pol_name = polAst->string.value;
                else if (polAst->type == NODE_IDENTIFIER)
                  pol_name = polAst->identifier.name;
                if (pol_name && strcmp(fe->key, pol_name) == 0 && exp->policies[pi].policy &&
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
          struct RuntimeObjectEntry *priv_entries = NULL;
          for (int pi = 0; pi < exp->policyCount; pi++) {
            if (exp->policies[pi].nameNode >= 0 && exp->policies[pi].nameNode < node->length &&
                exp->policies[pi].policy && strcmp(exp->policies[pi].policy, "private") == 0) {
              AstNode *polAst = &node->ast[exp->policies[pi].nameNode];
              const char *pol_name = NULL;
              if (polAst->type == NODE_LITERAL_ID)
                pol_name = polAst->string.value;
              else if (polAst->type == NODE_IDENTIFIER)
                pol_name = polAst->identifier.name;
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
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(ns_name);
          se->value = mod_val;
          se->next = entries;
          entries = se;
        }
      } else if (exp->selectiveItems >= 0 && exp->selectiveItems < node->length) {
        AstNode *items_node = &node->ast[exp->selectiveItems];
        if (items_node->type == NODE_ARRAY) {
          for (int i = 0; i < items_node->array.length; i++) {
            int nid = items_node->array.elements[i];
            if (nid < 0 || nid >= node->length) continue;
            AstNode *item = &node->ast[nid];
            const char *item_name = NULL;
            if (item->type == NODE_LITERAL_ID)
              item_name = item->string.value;
            else if (item->type == NODE_IDENTIFIER)
              item_name = item->identifier.name;
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
