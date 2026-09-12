#include <rupa.h>
#include "module.h"

/* Interpreter Dispatch — the main interpretNode switch/case dispatcher.
 * Module loading utilities are in loader.c. */

InterpreterResult interpretNode(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length) return resultNormal(valueNull());

  if (n->ast[id].type == NODE_PROGRAM) {
    RuntimeValue last = valueNull();
    for (AstDeclaration *d = n->ast[id].program.declarations; d; d = d->next) {
      InterpreterResult r = interpretNode(n, d->nodeId, e, x);
      last = r.value;
      if (r.flow != FLOW_NORMAL) return r;
    }
    return resultNormal(last);
  }

  switch (n->ast[id].type) {
  case NODE_IMPORT: {
    int expr_id = n->ast[id].module.value;
    int name_id = n->ast[id].module.name;

    if (name_id >= 0 && expr_id >= 0 && expr_id < n->length) {
      AstNode *mod_ast = &n->ast[name_id];
      const char *mod_name = NULL;
      if (mod_ast->type == NODE_LITERAL_ID)
        mod_name = mod_ast->string.value;
      else if (mod_ast->type == NODE_IDENTIFIER)
        mod_name = mod_ast->identifier.name;

      if (!mod_name) return resultNormal(valueNull());

      bool local_path = hasDotSlash(mod_name);
      RuntimeValue module_val;
      bool found = false;

      if (local_path) {
        module_val = loadModuleFile(mod_name, true);
        found = (module_val.type == VALUE_OBJECT);
        if (!found && n->ast[expr_id].type != NODE_ARRAY) {
          AstNode *tmp_ast = &n->ast[expr_id];
          const char *tmp_func = NULL;
          if (tmp_ast->type == NODE_LITERAL_ID)
            tmp_func = tmp_ast->string.value;
          else if (tmp_ast->type == NODE_IDENTIFIER)
            tmp_func = tmp_ast->identifier.name;
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
        if (strcmp(mod_name, "rupa") == 0) {
          if (n->ast[expr_id].type == NODE_ARRAY) {
            int len = n->ast[expr_id].array.length;
            for (int i = 0; i < len; i++) {
              int nid = n->ast[expr_id].array.elements[i];
              if (nid < 0 || nid >= n->length) continue;
              AstNode *na = &n->ast[nid];
              const char *fn = NULL;
              if (na->type == NODE_LITERAL_ID)
                fn = na->string.value;
              else if (na->type == NODE_IDENTIFIER)
                fn = na->identifier.name;
              if (!fn) continue;

              RuntimeValue pkg_val = valueNull();
              bool pkg_found = false;

              const char *pkg_path = stdlibFindModule(fn);
              if (pkg_path) {
                pkg_val = loadModuleFile(pkg_path, true);
                pkg_found = (pkg_val.type == VALUE_OBJECT);
              }
              if (!pkg_found) pkg_found = stdlibGetModule(fn, &pkg_val);

              if (pkg_found) semSet(e, fn, pkg_val);
            }
            return resultNormal(valueNull());
          }
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
            if (!found) found = stdlibGetModule(root_name, &module_val);
          }
        }

        if (!found) {
          found = stdlibGetModule(mod_name, &module_val);
          if (!found) {
            module_val = loadModuleFile(mod_name, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
        if (!found && n->ast[expr_id].type != NODE_ARRAY) {
          AstNode *tmp_ast = &n->ast[expr_id];
          const char *tmp_func = NULL;
          if (tmp_ast->type == NODE_LITERAL_ID)
            tmp_func = tmp_ast->string.value;
          else if (tmp_ast->type == NODE_IDENTIFIER)
            tmp_func = tmp_ast->identifier.name;
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
        if (!found && strncmp(mod_name, "rupa.", 5) == 0) {
          const char *pkg_name = mod_name + 5;
          const char *ext_path = stdlibFindModule(pkg_name);
          if (ext_path) {
            module_val = loadModuleFile(ext_path, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
        if (!found) {
          const char *ext_path = stdlibFindModule(mod_name);
          if (ext_path) {
            module_val = loadModuleFile(ext_path, true);
            found = (module_val.type == VALUE_OBJECT);
          }
        }
      }

      if (!found) return resultNormal(valueNull());

      bool rupa_root_import = strcmp(mod_name, "rupa") == 0;

      if (n->ast[expr_id].type == NODE_ARRAY) {
        int len = n->ast[expr_id].array.length;
        for (int i = 0; i < len; i++) {
          int nid = n->ast[expr_id].array.elements[i];
          if (nid < 0 || nid >= n->length) continue;
          AstNode *na = &n->ast[nid];
          const char *fn = NULL;
          if (na->type == NODE_LITERAL_ID)
            fn = na->string.value;
          else if (na->type == NODE_IDENTIFIER)
            fn = na->identifier.name;
          if (!fn) continue;

          if (rupa_root_import) {
            RuntimeValue pkg_val = valueNull();
            bool pkg_found = false;

            const char *pkg_path = stdlibFindModule(fn);
            if (pkg_path) {
              pkg_val = loadModuleFile(pkg_path, true);
              pkg_found = (pkg_val.type == VALUE_OBJECT);
            }
            if (!pkg_found) pkg_found = stdlibGetModule(fn, &pkg_val);

            if (pkg_found) semSet(e, fn, pkg_val);
          } else {
            RuntimeValue fn_val;
            if (valueObjectGet(module_val, fn, &fn_val)) semSet(e, fn, fn_val);
          }
        }
      } else {
        AstNode *func_ast = &n->ast[expr_id];
        const char *func_name = NULL;
        if (func_ast->type == NODE_LITERAL_ID)
          func_name = func_ast->string.value;
        else if (func_ast->type == NODE_IDENTIFIER)
          func_name = func_ast->identifier.name;
        if (func_name) {
          RuntimeValue fn_val;
          if (rupa_root_import) {
            semSet(e, func_name, module_val);
          } else if (valueObjectGet(module_val, func_name, &fn_val)) {
            semSet(e, func_name, fn_val);
          } else if (hasDotSlash(mod_name)) {
            semSet(e, func_name, module_val);
          }
        }
      }
    } else if (expr_id >= 0 && expr_id < n->length) {
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
            if (module_val.type == VALUE_OBJECT) semSet(e, module_name, module_val);
          } else {
            module_val = loadModuleFile(module_name, true);
            if (module_val.type == VALUE_OBJECT) semSet(e, module_name, module_val);
          }
        }
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_MODULE_IMPORT: {
    AstNode *importNode = &n->ast[id];
    int basePathId = importNode->moduleImport.basePath;
    struct AstModuleImportEntry *entries = importNode->moduleImport.entries;
    int entryCount = importNode->moduleImport.entryCount;
    int aliasId = importNode->moduleImport.alias;

    if (basePathId < 0 || basePathId >= n->length) return resultNormal(valueNull());

    const char *base_path = NULL;
    AstNode *baseAst = &n->ast[basePathId];
    if (baseAst->type == NODE_LITERAL_ID)
      base_path = baseAst->string.value;
    else if (baseAst->type == NODE_IDENTIFIER)
      base_path = baseAst->identifier.name;
    if (!base_path) return resultNormal(valueNull());

    const char *ns_name = base_path;
    if (aliasId >= 0 && aliasId < n->length) {
      AstNode *aliasAst = &n->ast[aliasId];
      if (aliasAst->type == NODE_LITERAL_ID)
        ns_name = aliasAst->string.value;
      else if (aliasAst->type == NODE_IDENTIFIER)
        ns_name = aliasAst->identifier.name;
    }

    struct RuntimeObjectEntry *nsEntries = NULL;
    for (int i = 0; i < entryCount; i++) {
      struct AstModuleImportEntry *e = &entries[i];

      const char *path_str = NULL;
      if (e->pathNode >= 0 && e->pathNode < n->length) {
        AstNode *pathAst = &n->ast[e->pathNode];
        if (pathAst->type == NODE_LITERAL_ID)
          path_str = pathAst->string.value;
        else if (pathAst->type == NODE_IDENTIFIER)
          path_str = pathAst->identifier.name;
      }
      if (!path_str) continue;

      const char *alias_str = NULL;
      if (e->aliasNode >= 0 && e->aliasNode < n->length) {
        AstNode *aliasAst = &n->ast[e->aliasNode];
        if (aliasAst->type == NODE_LITERAL_ID)
          alias_str = aliasAst->string.value;
        else if (aliasAst->type == NODE_IDENTIFIER)
          alias_str = aliasAst->identifier.name;
      }

      char file_path[512];
      const char *dot = strchr(path_str, '.');
      if (dot && !e->isWildcard) {
        int prefix_len = (int)(dot - path_str);
        snprintf(file_path, sizeof(file_path), "%s/%.*s", base_path, prefix_len, path_str);
      } else {
        snprintf(file_path, sizeof(file_path), "%s/%s", base_path, path_str);
      }

      RuntimeValue mod_val = loadModuleFile(file_path, true);
      if (mod_val.type != VALUE_OBJECT) continue;

      const char *key_name = alias_str ? alias_str : path_str;
      if (dot && !e->isWildcard) {
        key_name = alias_str ? alias_str : dot + 1;
      }

      if (e->isWildcard) {
        if (alias_str) {
          struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
          se->key = strdup(alias_str);
          se->value = mod_val;
          se->next = nsEntries;
          nsEntries = se;
        } else {
          for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries; fe; fe = fe->next) {
            struct RuntimeObjectEntry *se = calloc(1, sizeof(*se));
            se->key = strdup(fe->key);
            se->value = fe->value;
            se->next = nsEntries;
            nsEntries = se;
          }
        }
      } else {
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

    if (aliasId >= 0) {
      semSet(e, ns_name, valueObject(nsEntries));
    } else {
      struct RuntimeObjectEntry *se = nsEntries;
      while (se) {
        semSet(e, se->key, se->value);
        se = se->next;
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_EXPORT: {
    return resultNormal(valueNull());
  }
  case NODE_EXPORT_DECL: {
    {
      AstNode *expNode = &n->ast[id];
      struct AstExport *exp = &expNode->astExport;

      const char *src_path = NULL;
      if (exp->sourcePath >= 0 && exp->sourcePath < n->length) {
        AstNode *srcAst = &n->ast[exp->sourcePath];
        if (srcAst->type == NODE_LITERAL_ID)
          src_path = srcAst->string.value;
        else if (srcAst->type == NODE_IDENTIFIER)
          src_path = srcAst->identifier.name;
      }
      if (!src_path) return resultNormal(valueNull());

      RuntimeValue mod_val = loadModuleFile(src_path, false);

      if (exp->namespaceName >= 0 && exp->namespaceName < n->length) {
        const char *ns_name = NULL;
        AstNode *nsAst = &n->ast[exp->namespaceName];
        if (nsAst->type == NODE_LITERAL_ID)
          ns_name = nsAst->string.value;
        else if (nsAst->type == NODE_IDENTIFIER)
          ns_name = nsAst->identifier.name;
        if (ns_name && mod_val.type == VALUE_OBJECT) {
          if (exp->policyCount > 0 && exp->policies) {
            struct RuntimeObjectEntry *filtered = NULL;
            for (struct RuntimeObjectEntry *fe = mod_val.as.object.entries; fe; fe = fe->next) {
              bool is_private = false;
              for (int pi = 0; pi < exp->policyCount; pi++) {
                if (exp->policies[pi].nameNode >= 0 && exp->policies[pi].nameNode < n->length) {
                  AstNode *polAst = &n->ast[exp->policies[pi].nameNode];
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
            semSet(e, ns_name, valueObject(filtered));
          } else {
            semSet(e, ns_name, mod_val);
          }
        }
      } else if (exp->selectiveItems >= 0 && exp->selectiveItems < n->length) {
        AstNode *items_node = &n->ast[exp->selectiveItems];
        if (items_node->type == NODE_ARRAY && mod_val.type == VALUE_OBJECT) {
          for (int i = 0; i < items_node->array.length; i++) {
            int nid = items_node->array.elements[i];
            if (nid < 0 || nid >= n->length) continue;
            AstNode *item = &n->ast[nid];
            const char *item_name = NULL;
            if (item->type == NODE_LITERAL_ID)
              item_name = item->string.value;
            else if (item->type == NODE_IDENTIFIER)
              item_name = item->identifier.name;
            if (!item_name) continue;
            RuntimeValue item_val;
            if (valueObjectGet(mod_val, item_name, &item_val)) semSet(e, item_name, item_val);
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
  case NODE_COMMENT:
  case NODE_INLINE_COMMENT:
  case NODE_BLOCK_COMMENT:
    /* Comments are parsed into the AST but ignored by the interpreter. */
    return resultNormal(valueNull());
  default:
    return interpretExpression(n, id, e, x);
  }
}
