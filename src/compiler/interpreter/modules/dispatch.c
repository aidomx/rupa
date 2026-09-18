#include <rupa.h>
/* Interpreter Dispatch — the main interpretNode switch/case dispatcher.
 * Module loading utilities are in loader.c. */
#include "module.h"
/* ==================== NODE_MOD helpers ==================== */

/* Flatten an entry to its dotted path string: "a.create", "b.*". */
static const char *modEntryPathStr(AstModEntry *e, char *buf, int size) {
  if (!e || !e->name) return NULL;
  snprintf(buf, size, "%s", e->name);
  if (e->type == MOD_WILD) strncat(buf, ".*", size - (int)strlen(buf) - 1);
  for (AstModEntry *c = e->childrens; c && c->name; c = c->childrens) {
    strncat(buf, ".", size - (int)strlen(buf) - 1);
    strncat(buf, c->name, size - (int)strlen(buf) - 1);
  }
  return buf;
}

/* Resolve a source string to a module object.
 *   "./x.y" / "../x"  → local file load (whole env, export not required)
 *   "rupa.M"          → stdlib module M
 *   "rupa"            → handled per-entry by caller
 *   "name"            → stdlib lookup first, then local file
 */
static RuntimeValue modLoadSource(const char *source) {
  RuntimeValue v = valueNull();
  if (!source) return v;

  if (hasDotSlash(source)) return loadModuleFile(source, false);

  if (strncmp(source, "rupa.", 5) == 0) {
    const char *pkg = source + 5;
    const char *ext = stdlibFindModule(pkg);
    if (ext) {
      v = loadModuleFile(ext, false);
      if (v.type == VALUE_OBJECT) return v;
    }
    if (stdlibGetModule(pkg, &v)) return v;
    return valueNull();
  }

  const char *ext = stdlibFindModule(source);
  if (ext) {
    v = loadModuleFile(ext, false);
    if (v.type == VALUE_OBJECT) return v;
  }
  if (stdlibGetModule(source, &v)) return v;
  return loadModuleFile(source, false);
}

static void modAppendEntry(struct RuntimeObjectEntry **list, const char *key, RuntimeValue v) {
  struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
  se->key = gcstrdup(key);
  se->value = v;
  se->next = *list;
  *list = se;
}

/* Build a filtered copy of module object entries per `-> { name: private }`. */
static RuntimeValue modFilterPolicies(struct AstMod *mod, RuntimeValue mv) {
  struct RuntimeObjectEntry *filtered = NULL;
  for (struct RuntimeObjectEntry *fe = mv.as.object.entries; fe; fe = fe->next) {
    bool is_private = false;
    for (int pi = 0; pi < mod->policyCount; pi++) {
      if (mod->policies[pi].name && strcmp(fe->key, mod->policies[pi].name) == 0 &&
          mod->policies[pi].value && strcmp(mod->policies[pi].value, "private") == 0) {
        is_private = true;
        break;
      }
    }
    if (!is_private) modAppendEntry(&filtered, fe->key, fe->value);
  }
  return valueObject(filtered);
}

/* Interpret ImportDecl: bind entries from source into env.
 * With sourceAlias, bindings collect into a namespace object instead. */
static InterpreterResult interpretModImport(Node *n, int id, RuntimeEnv *e) {
  struct AstMod *mod = &n->ast[id].mod;

  /* Bare import `import X` (no source) → bind module X directly. */
  if (!mod->source) {
    for (int i = 0; i < mod->entryCount; i++) {
      char buf[512];
      const char *name = modEntryPathStr(&mod->entries[i], buf, sizeof(buf));
      if (!name) continue;
      RuntimeValue v = modLoadSource(name);
      if (v.type == VALUE_OBJECT) semSet(e, name, v);
    }
    return resultNormal(valueNull());
  }

  bool rupa_root = strcmp(mod->source, "rupa") == 0;
  struct RuntimeObjectEntry *nsEntries = NULL;

  for (int i = 0; i < mod->entryCount; i++) {
    AstModEntry *en = &mod->entries[i];
    char pathBuf[512];
    const char *pathStr = modEntryPathStr(en, pathBuf, sizeof(pathBuf));
    if (!pathStr) continue;
    const char *bindName = en->key ? en->key : en->name;
    if (!en->key && en->type == MOD_MEMBER && en->childrens) {
      /* `b.login` (no alias) → bind under the last segment's name
       * ("login"), not the module prefix ("b"). */
      AstModEntry *last = en->childrens;
      while (last->childrens)
        last = last->childrens;
      if (last->name) bindName = last->name;
    }

    /* `import X from rupa` → load each name as its own stdlib module */
    if (rupa_root) {
      RuntimeValue pkg = valueNull();
      bool ok = false;
      bool namespace_pkg = false;
      const char *pkg_path = stdlibFindModule(en->name);
      if (pkg_path) {
        pkg = loadModuleFile(pkg_path, false);
        ok = (pkg.type == VALUE_OBJECT);
      }
      if (!ok) {
        const char *ns_path = stdlibFindNamespace(en->name);
        if (ns_path) {
          pkg = loadModuleFile(ns_path, false);
          ok = (pkg.type == VALUE_OBJECT);
          namespace_pkg = ok;
        }
      }
      if (!ok) ok = stdlibGetModule(en->name, &pkg);
      if (ok) {
        /* A package index may expose a namespace whose public name differs
         * from the package directory, e.g. database/index.rp -> namespace db.
         * loadModuleFile() returns the index environment, so unwrap that
         * namespace before binding `db`. */
        RuntimeValue bindValue = pkg;
        if (namespace_pkg) {
          RuntimeValue ns;
          if (valueObjectGet(pkg, en->name, &ns)) bindValue = ns;
        }

        if (mod->sourceAlias)
          modAppendEntry(&nsEntries, bindName, bindValue);
        else
          semSet(e, bindName, bindValue);
      }
      continue;
    }

    /* Wildcard: `d.*` → load d.rp and bind it / flatten its entries. */
    if (en->type == MOD_WILD) {
      RuntimeValue mv = valueNull();
      {
        char fileBuf[512];
        snprintf(fileBuf, sizeof(fileBuf), "%s/%s", mod->source, en->name);
        mv = loadModuleFile(fileBuf, true);
      }
      if (mv.type != VALUE_OBJECT) mv = modLoadSource(mod->source);
      if (mv.type != VALUE_OBJECT) continue;

      if (en->key) {
        /* `a.* as form` → bind whole module object */
        if (mod->sourceAlias)
          modAppendEntry(&nsEntries, bindName, mv);
        else
          semSet(e, bindName, mv);
      } else {
        /* `d.*` → flatten module entries into env/namespace */
        for (struct RuntimeObjectEntry *fe = mv.as.object.entries; fe; fe = fe->next) {
          if (mod->sourceAlias)
            modAppendEntry(&nsEntries, fe->key, fe->value);
          else
            semSet(e, fe->key, fe->value);
        }
      }
      continue;
    }

    /* ID / MEMBER entries */
    RuntimeValue mv = valueNull();
    bool loaded = false;

    /* Entry with alias or sub-path prefers `<source>/<name>` file first:
     * `a as form` → a.rp, `b.login` → b.rp. */
    if (en->key || en->type == MOD_MEMBER) {
      char fileBuf[512];
      snprintf(fileBuf, sizeof(fileBuf), "%s/%s", mod->source, en->name);
      mv = loadModuleFile(fileBuf, true);
      loaded = (mv.type == VALUE_OBJECT);
    }
    if (!loaded) {
      mv = modLoadSource(mod->source);
      loaded = (mv.type == VALUE_OBJECT);
    }
    if (!loaded) {
      /* `import d from ../modules` → try ../modules.d (d.rp) */
      char fileBuf[512];
      snprintf(fileBuf, sizeof(fileBuf), "%s.%s", mod->source, en->name);
      mv = loadModuleFile(fileBuf, true);
      loaded = (mv.type == VALUE_OBJECT);
    }
    if (!loaded) continue;

    if (en->type == MOD_MEMBER && en->childrens) {
      /* `b.login` → dig into childrens chain */
      RuntimeValue cur = mv;
      bool found = true;
      for (AstModEntry *c = en->childrens; c && c->name; c = c->childrens) {
        RuntimeValue nv;
        if (cur.type == VALUE_OBJECT && valueObjectGet(cur, c->name, &nv)) {
          cur = nv;
        } else {
          found = false;
          break;
        }
      }
      if (found) {
        if (mod->sourceAlias)
          modAppendEntry(&nsEntries, bindName, cur);
        else
          semSet(e, bindName, cur);
      }
    } else if (en->key) {
      /* `a as form` → bind whole module object */
      if (mod->sourceAlias)
        modAppendEntry(&nsEntries, bindName, mv);
      else
        semSet(e, bindName, mv);
    } else {
      /* plain `X from path` → member X, fallback whole module for local */
      RuntimeValue fn_val;
      if (valueObjectGet(mv, en->name, &fn_val)) {
        if (mod->sourceAlias)
          modAppendEntry(&nsEntries, bindName, fn_val);
        else
          semSet(e, bindName, fn_val);
      } else if (hasDotSlash(mod->source)) {
        if (mod->sourceAlias)
          modAppendEntry(&nsEntries, bindName, mv);
        else
          semSet(e, bindName, mv);
      }
    }
  }

  if (mod->sourceAlias) semSet(e, mod->sourceAlias, valueObject(nsEntries));
  return resultNormal(valueNull());
}

/* Resolve one implicit-file export entry (no `from` on the AstMod): the
 * entry's own name is a file relative to the current directory (same
 * resolution loadModuleFile already does via g_source_file_path), reusing
 * modLoadSource so implicit lookups behave identically to explicit `from`
 * ones. MOD_MEMBER entries drill into en->childrens (mirrors the
 * member-chain walk on the import side); MOD_ID/MOD_WILD bind the whole
 * loaded file. Appends (bindName, value) to *out instead of binding
 * anywhere, so callers decide where it lands (env directly, or merged into
 * a namespace object first). */
static void resolveImplicitExportEntry(AstModEntry *en, struct RuntimeObjectEntry **out) {
  if (!en || !en->name) return;
  RuntimeValue mv = modLoadSource(en->name);
  if (mv.type != VALUE_OBJECT) return;

  if (en->type == MOD_MEMBER && en->childrens) {
    RuntimeValue cur = mv;
    bool found = true;
    for (AstModEntry *c = en->childrens; c && c->name; c = c->childrens) {
      RuntimeValue nv;
      if (cur.type == VALUE_OBJECT && valueObjectGet(cur, c->name, &nv)) {
        cur = nv;
      } else {
        found = false;
        break;
      }
    }
    if (!found) return;
    /* Bind name defaults to the last chain segment ("connect"), not the
     * file name ("driver") — same rule as the import-side fix. */
    AstModEntry *last = en->childrens;
    while (last->childrens)
      last = last->childrens;
    const char *bindName = en->key ? en->key : (last->name ? last->name : en->name);
    modAppendEntry(out, bindName, cur);
    return;
  }

  modAppendEntry(out, en->key ? en->key : en->name, mv);
}

/* Compute (bindName, value) pairs for one ExportDecl without binding them
 * anywhere — shared by top-level export (bound straight to env) and
 * namespace blocks (merged, deduped, then bound once under the namespace
 * name).
 *
 * `insideNamespace` controls bare MOD_ID entries with no `from`: at top
 * level, `export x` (bare, no dot) stays a pure local marker — a contract
 * for the loader (see loader.c), not something with any own runtime value —
 * so it's skipped, preserving existing behavior. Inside a `namespace`
 * block, every bare name is implicitly a sibling file
 * (`namespace db { export driver }` → load ./driver.rp, bind as "driver"),
 * since referencing other files is the entire point of the block. */
static void computeExportBindings(struct AstMod *mod, bool insideNamespace,
                                  struct RuntimeObjectEntry **out) {
  if (!mod->source) {
    for (int i = 0; i < mod->entryCount; i++) {
      AstModEntry *en = &mod->entries[i];
      if (en->type == MOD_ID && !insideNamespace) continue; /* local marker, no-op */
      resolveImplicitExportEntry(en, out);
    }
    return;
  }

  RuntimeValue mv = modLoadSource(mod->source);
  if (mv.type != VALUE_OBJECT) return;

  /* Single plain entry = namespace re-export (any alias, whole module) */
  bool namespace_mode =
      (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_ID);

  if (namespace_mode) {
    const char *ns = mod->entries[0].name;
    if (ns) {
      RuntimeValue obj = (mod->policyCount > 0 && mod->policies) ? modFilterPolicies(mod, mv) : mv;
      modAppendEntry(out, ns, obj);
    }
    return;
  }

  /* Selective: bind each entry member (dotted chains included) */
  for (int i = 0; i < mod->entryCount; i++) {
    AstModEntry *en = &mod->entries[i];
    if (!en->name) continue;

    if (en->type == MOD_MEMBER && en->childrens) {
      RuntimeValue cur = mv;
      bool found = true;
      for (AstModEntry *c = en->childrens; c && c->name; c = c->childrens) {
        RuntimeValue nv;
        if (cur.type == VALUE_OBJECT && valueObjectGet(cur, c->name, &nv)) {
          cur = nv;
        } else {
          found = false;
          break;
        }
      }
      if (found) {
        AstModEntry *last = en->childrens;
        while (last->childrens)
          last = last->childrens;
        modAppendEntry(out, en->key ? en->key : (last->name ? last->name : en->name), cur);
      }
      continue;
    }

    RuntimeValue item;
    if (valueObjectGet(mv, en->name, &item))
      modAppendEntry(out, en->key ? en->key : en->name, item);
  }
}

/* Free a RuntimeObjectEntry list's spine (keys/nodes only — values are
 * shared RuntimeValue payloads owned elsewhere, e.g. GC'd module objects). */
static void freeBindingList(struct RuntimeObjectEntry *list) {
  while (list) {
    struct RuntimeObjectEntry *next = list->next;
    gcfree(list->key);
    gcfree(list);
    list = next;
  }
}

/* Interpret ExportDecl at top level: compute bindings and set them directly
 * into env. `export x` (bare, no `from`) stays a no-op marker for the
 * loader; everything else (re-export, or an implicit-file dotted/wildcard
 * export) actually binds. */
static void interpretModExport(Node *n, int id, RuntimeEnv *e) {
  struct AstMod *mod = &n->ast[id].mod;
  struct RuntimeObjectEntry *out = NULL;
  computeExportBindings(mod, /*insideNamespace=*/false, &out);
  for (struct RuntimeObjectEntry *o = out; o; o = o->next)
    semSet(e, o->key, o->value);
  freeBindingList(out);
}

/* Interpret NamespaceDecl: `namespace db { export ...; export ...; }`.
 * Runs each nested ExportDecl in the block (as an implicit-file export —
 * see computeExportBindings), merges the results into one object, and binds
 * it once under the namespace name. Errors on duplicate bind-names within
 * the same block: since bare names nest by construction (`db.driver`,
 * `db.table` never collide), a collision only happens when the user
 * explicitly chose the same alias/name twice at the same level. */
static bool interpretModNamespace(Node *n, int id, RuntimeEnv *e, Error *x) {
  struct AstMod *mod = &n->ast[id].mod;
  const char *nsName = mod->source;
  if (!nsName || mod->body < 0 || mod->body >= n->length) return false;

  AstNode *block = &n->ast[mod->body];
  if (block->type != NODE_BLOCK) return false;

  struct RuntimeObjectEntry *merged = NULL;
  bool hadError = false;

  for (int i = 0; i < block->block.length; i++) {
    int stmtId = block->block.statements[i];
    if (stmtId < 0 || stmtId >= n->length) continue;
    AstNode *stmt = &n->ast[stmtId];
    if (stmt->type != NODE_MOD || stmt->mod.type != ExportDecl) continue;

    struct RuntimeObjectEntry *out = NULL;
    computeExportBindings(&stmt->mod, /*insideNamespace=*/true, &out);

    struct RuntimeObjectEntry *o = out;
    while (o) {
      struct RuntimeObjectEntry *next = o->next;
      bool dup = false;
      for (struct RuntimeObjectEntry *m = merged; m; m = m->next) {
        if (strcmp(m->key, o->key) == 0) {
          dup = true;
          break;
        }
      }
      if (dup) {
        hadError = true;
        if (x) {
          static char message[256];
          snprintf(message, sizeof(message), "Duplicate export '%s' in namespace '%s'", o->key,
                   nsName);
          addError(x, (ErrorInfo){.code = "ExportError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_REDECLARED_VAR});
        }
        gcfree(o->key);
        gcfree(o);
      } else {
        o->next = merged;
        merged = o;
      }
      o = next;
    }
  }

  semSet(e, nsName, valueObject(merged));
  return hadError;
}

/* ==================== Main dispatch ==================== */

InterpreterResult interpretNode(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length) return resultNormal(valueNull());

  setRuntimeErrorLocation(n->ast[id].line, n->ast[id].row);

  if (n->ast[id].type == NODE_PROGRAM) {
    RuntimeValue last = valueNull();
    for (AstDeclaration *d = n->ast[id].program.declarations; d; d = d->next) {
      InterpreterResult r = interpretNode(n, d->nodeId, e, x);
      last = r.value;
      if (r.flow == FLOW_RETURN || r.flow == FLOW_BREAK || r.flow == FLOW_CONTINUE) return r;
      /* FLOW_ERROR is recoverable: the error is already recorded in x.
       * Continue executing the remaining top-level declarations. */
    }
    return resultNormal(last);
  }

  switch (n->ast[id].type) {
  case NODE_MOD: {
    AstNode *self = &n->ast[id];
    if (self->mod.type == ImportDecl) return interpretModImport(n, id, e);
    if (self->mod.type == NamespaceDecl) {
      bool hadError = interpretModNamespace(n, id, e, x);
      if (hadError) return resultFlow(FLOW_ERROR, valueNull());
      return resultNormal(valueNull());
    }
    interpretModExport(n, id, e);
    return resultNormal(valueNull());
  }
  case NODE_EXTENDS:
    /* Reserved for future inheritance (warisan class/activity). */
    return resultNormal(valueNull());
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
  case NODE_CLASS_DECL:
  case NODE_MARKER: /* @created — dievaluasi via interpretStatement (no-op di sini) */
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
