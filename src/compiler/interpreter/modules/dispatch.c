#include <rupa.h>
#include <stdarg.h>
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
static RuntimeValue modLoadSource(const char *source, Error *error) {
  RuntimeValue v = valueNull();
  if (!source) return v;

  if (hasDotSlash(source)) return loadModuleFileError(source, false, error);

  if (strncmp(source, "rupa.", 5) == 0) {
    const char *pkg = source + 5;
    const char *ext = stdlibFindModule(pkg);
    if (ext) {
      v = loadModuleFileError(ext, false, error);
      if (v.type == VALUE_OBJECT) return v;
    }
    if (stdlibGetModule(pkg, &v)) return v;
    return valueNull();
  }

  const char *ext = stdlibFindModule(source);
  if (ext) {
    v = loadModuleFileError(ext, false, error);
    if (v.type == VALUE_OBJECT) return v;
  }
  if (stdlibGetModule(source, &v)) return v;
  return loadModuleFileError(source, false, error);
}

static void modAppendEntry(struct RuntimeObjectEntry **list, const char *key, RuntimeValue v) {
  struct RuntimeObjectEntry *se = gccalloc(1, sizeof(*se));
  se->key = gcstrdup(key);
  se->value = v;
  se->next = *list;
  *list = se;
}

/* ImportError spesifik: import yang gagal (module tanpa export, member
 * tidak ada di module, module tidak ditemukan) dilaporkan ke caller —
 * bukan diam-diam di-skip lalu terbaca `undefined`. Line/row dikirim
 * eksplisit dari node import: global lokasi error pasti sudah tertimpa
 * eksekusi statement terakhir di dalam module yang dimuat. */
static void addImportError(Error *x, int line, int row, const char *fmt, ...) {
  if (!x) return;
  static char message[MAX_MESSAGE_LENGTH];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  addError(x, (ErrorInfo){.code = "ImportError",
                          .message = message,
                          .line = line,
                          .row = row,
                          .type = ERR});
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

/* Lokasi statement import — dipakai semua pesan ImportError di bawah
 * supaya error menunjuk baris `import ...`, bukan posisi terakhir di
 * dalam module yang sedang dimuat. */
#define IMP_LOC n->ast[id].line, n->ast[id].row

/* Binding hasil `import` — hanya module OBJECT (whole-env / namespace
 * dari file lain) yang ditandai hidden di whole-env export. Member leaf
 * (fungsi/nilai, mis. `import has, get from ./drivers`) TIDAK ditandai:
 * itu pola komposisi package yang memang harus terekspos, sama seperti
 * hasil wildcard flatten. */
static void modBind(RuntimeEnv *e, const char *name, RuntimeValue v) {
  semSet(e, name, v);
  if (v.type == VALUE_OBJECT) semMarkImport(e, name);
}

/* Source navigasi pohon: `./x.y`, `../a.b`, `./a.b.c` — bagian member
 * (setelah prefix ./|../) mengandung '.'. Member lookup di index parent
 * WAJIB cocok; tanpa fallback whole-module (design/import_export.txt:
 * "apakah ada sub module yang di export?"). */
static bool modSourceIsTreeNav(const char *src) {
  if (!src) return false;
  const char *p = src;
  if (p[0] == '.' && p[1] == '.' && p[2] == '/') p += 3;
  else if (p[0] == '.' && p[1] == '/') p += 2;
  return strchr(p, '.') != NULL;
}

/* Interpret ImportDecl: bind entries from source into env.
 * With sourceAlias, bindings collect into a namespace object instead. */
static InterpreterResult interpretModImport(Node *n, int id, RuntimeEnv *e, Error *x) {
  struct AstMod *mod = &n->ast[id].mod;

  /* Bare import: `import name` — hanya sejajar (satu folder,
   * design/import_export.txt §Bare). Muat ./<name>.rp atau
   * ./<name>/index.rp dan bind dengan nama yang sama. */
  if (!mod->source) {
    if (mod->entryCount != 1) return resultNormal(valueNull());
    AstModEntry *ben = &mod->entries[0];
    if (!ben->name || ben->type != MOD_ID) return resultNormal(valueNull());

    char barePath[64];
    snprintf(barePath, sizeof(barePath), "./%s", ben->name);
    RuntimeValue mv = modLoadSource(barePath, x);
    if (mv.type != VALUE_OBJECT) {
      addImportError(x, IMP_LOC,
                     "Cannot import '%s' — no './%s.rp' or './%s/index.rp' beside this file",
                     ben->name, ben->name, ben->name);
      return resultNormal(valueNull());
    }
    /* ie.txt import #1: member kalau ada, selain itu whole module =
     * namespace. Aturan seragam yang sama dengan bentuk `from`. */
    RuntimeValue fn_val;
    const char *bindName = ben->key ? ben->key : ben->name;
    if (valueObjectGet(mv, ben->name, &fn_val))
      modBind(e, bindName, fn_val);
    else
      modBind(e, bindName, mv);
    return resultNormal(valueNull());
  }

  bool rupa_root = strcmp(mod->source, "rupa") == 0;

  for (int i = 0; i < mod->entryCount; i++) {
    AstModEntry *en = &mod->entries[i];
    char pathBuf[512];
    const char *pathStr = modEntryPathStr(en, pathBuf, sizeof(pathBuf));
    if (!pathStr) continue;
    const char *bindName = en->key ? en->key : en->name;

    /* `import X from rupa` → load each name as its own stdlib module */
    if (rupa_root) {
      RuntimeValue pkg = valueNull();
      bool ok = false;
      bool namespace_pkg = false;
      const char *pkg_path = stdlibFindModule(en->name);
      if (pkg_path) {
        pkg = loadModuleFileError(pkg_path, false, x);
        ok = (pkg.type == VALUE_OBJECT);
      }
      if (!ok) {
        const char *ns_path = stdlibFindNamespace(en->name);
        if (ns_path) {
          pkg = loadModuleFileError(ns_path, false, x);
          ok = (pkg.type == VALUE_OBJECT);
          namespace_pkg = ok;
        }
      }
      if (!ok) ok = stdlibGetModule(en->name, &pkg); /* native module (.o) */
      if (!ok) {
        addImportError(x, IMP_LOC, "Stdlib package or namespace '%s' not found in 'rupa'", en->name);
        continue;
      }
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

        modBind(e, bindName, bindValue);
      }
      continue;
    }

    /* Wildcard: leading star `import * as x from ./X` — seluruh member
     * source (bentuk kanonik). Entry bernama kosong. */
    if (en->type == MOD_WILD) {
      RuntimeValue whole = modLoadSource(mod->source, x);
      if (whole.type != VALUE_OBJECT) {
        addImportError(
            x, IMP_LOC, "Module '%s' not found, empty, or does not export anything", mod->source);
        continue;
      }
      if (en->key) {
        /* `* as x` → bind whole module object */
        modBind(e, en->key, whole);
      } else {
        /* `*` tanpa alias → flatten (publik, pola komposisi) */
        for (struct RuntimeObjectEntry *fe = whole.as.object.entries; fe; fe = fe->next)
          semSet(e, fe->key, fe->value);
      }
      continue;
    }

    /* ID entries: member dari source, fallback whole-module untuk
     * local path, lalu fallback `<source>.<name>` (d.rp). */
    RuntimeValue mv = modLoadSource(mod->source, x);
    if (mv.type != VALUE_OBJECT) {
      char fileBuf[512];
      snprintf(fileBuf, sizeof(fileBuf), "%s.%s", mod->source, en->name);
      mv = loadModuleFile(fileBuf, true);
    }
    if (mv.type != VALUE_OBJECT) {
      addImportError(
          x, IMP_LOC,
          "Import failed for '%s' from '%s' — module not found or does not export anything",
          en->name, mod->source);
      continue;
    }

    RuntimeValue fn_val;
    if (valueObjectGet(mv, en->name, &fn_val)) {
      modBind(e, bindName, fn_val);
    } else if (mod->entryCount == 1 && modSourceResolvesLeaf(mod->source)) {
      /* ie.txt import #4: single entry tanpa member yang cocok otomatis
       * menjadi namespace (whole module). Hanya untuk leaf sungguhan —
       * path yang di-resolve loader ke parent index (tree nav #5) wajib
       * menemukan member di sana. */
      modBind(e, bindName, mv);
    } else {
      addImportError(x, IMP_LOC, "Member '%s' not found in module '%s'", pathStr, mod->source);
    }
  }

  return resultNormal(valueNull());
}

/* Compute (bindName, value) pairs for one ExportDecl without binding them
 * anywhere — shared by top-level export (bound straight to env) and
 * namespace blocks (merged, deduped, then bound once under the namespace
 * name).
 *
 * Bare export (tanpa `from`, design/import_export.txt §Bare) — siklus
 * sejajar:
 *   export x, y → ikat member env saat ini (diri sendiri)
 *   export X    → X.rp sejajar ada → re-export whole module;
 *                 tidak ada → X adalah member env ini.
 * Urutan: member sendiri menang, lalu file sejajar. */
static void computeExportBindings(RuntimeEnv *env, struct AstMod *mod,
                                  struct RuntimeObjectEntry **out, Error *error) {
  /* `export *` (ie.txt export #1): semua milik file ini. Binding dibaca
   * di posisi statement export — bukan snapshot whole-env yang dihitung
   * loader hanya SETELAH semua statement selesai. */
  if (!mod->source && mod->entryCount == 1 && mod->entries[0].type == MOD_WILD) {
    for (RuntimeBinding *b = env->bindings; b; b = b->next) {
      if (strcmp(b->name, "AWAIT") == 0 || strcmp(b->name, "SUCCESS") == 0 ||
          strcmp(b->name, "ERROR") == 0)
        continue;
      if (b->value.type == VALUE_NATIVE_FUNCTION) continue;
      if (b->isImport) continue;
      modAppendEntry(out, b->name, b->value);
    }
    return;
  }

  if (!mod->source) {
    for (int i = 0; i < mod->entryCount; i++) {
      AstModEntry *en = &mod->entries[i];
      if (!en->name || en->type != MOD_ID) continue;
      RuntimeValue self_val;
      if (semGet(env, en->name, &self_val)) {
        modAppendEntry(out, en->key ? en->key : en->name, self_val);
        continue;
      }
      /* ie.txt export #3: tidak ada di file ini → otomatis namespace
       * dari file sejajar (re-export whole module). */
      char barePath[64];
      snprintf(barePath, sizeof(barePath), "./%s", en->name);
      RuntimeValue mv = modLoadSource(barePath, error);
      if (mv.type == VALUE_OBJECT) {
        modAppendEntry(out, en->key ? en->key : en->name, mv);
        continue;
      }
      addImportError(error, 0, 0,
                     "Cannot export '%s' — not defined here and no './%s.rp' beside this file",
                     en->name, en->name);
    }
    return;
  }

  RuntimeValue mv = modLoadSource(mod->source, error);
  if (mv.type != VALUE_OBJECT) return;

  /* Leading star: `export * from ./X` — ekspor seluruh member source
   * (bentuk kanonik design/import_export.txt). Entry bernama kosong. */
  if (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_WILD &&
      mod->entries[0].name && mod->entries[0].name[0] == '\0') {
    /* Policy private tetap ditegakkan pada hasil flatten. */
    RuntimeValue src = (mod->policyCount > 0 && mod->policies) ? modFilterPolicies(mod, mv) : mv;
    if (src.type == VALUE_OBJECT) {
      for (struct RuntimeObjectEntry *fe = src.as.object.entries; fe; fe = fe->next)
        modAppendEntry(out, fe->key, fe->value);
    }
    return;
  }

  /* Single plain entry = re-export. Member-first: bila module sumber
   * punya binding bernama sama (mis. fungsi `open` di open.rp), ikat
   * member itu; selain itu ikat seluruh module (re-export leaf/
   * sub-package, mis. `export resources from ./resources`). */
  bool namespace_mode =
      (mod->entryCount == 1 && !mod->entries[0].key && mod->entries[0].type == MOD_ID);
  bool tree_nav = modSourceIsTreeNav(mod->source);

  if (namespace_mode) {
    const char *ns = mod->entries[0].name;
    if (ns) {
      RuntimeValue member;
      if (valueObjectGet(mv, ns, &member)) {
        modAppendEntry(out, ns, member);
        return;
      }
      if (tree_nav) {
        /* Tree navigation: member wajib ada di index parent — cabang
         * yang tidak di-export tidak boleh diam-diam menjadi whole
         * module (design/import_export.txt). */
        addImportError(error, 0, 0,
                       "Export failed for '%s' from '%s' — member not found in parent index",
                       ns, mod->source);
        return;
      }
      RuntimeValue obj = (mod->policyCount > 0 && mod->policies) ? modFilterPolicies(mod, mv) : mv;
      modAppendEntry(out, ns, obj);
    }
    return;
  }

  /* Selective: bind each named member */
  for (int i = 0; i < mod->entryCount; i++) {
    AstModEntry *en = &mod->entries[i];
    if (!en->name) continue;
    RuntimeValue item;
    if (valueObjectGet(mv, en->name, &item)) {
      modAppendEntry(out, en->key ? en->key : en->name, item);
    } else if (tree_nav) {
      addImportError(error, 0, 0,
                     "Export failed for '%s' from '%s' — member not found in parent index",
                     en->name, mod->source);
    }
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
 * into env. Parser hanya menghasilkan export dengan `from`; guard source
 * NULL di computeExportBindings tinggal no-op. */
static void interpretModExport(Node *n, int id, RuntimeEnv *e, Error *x) {
  struct AstMod *mod = &n->ast[id].mod;
  struct RuntimeObjectEntry *out = NULL;
  computeExportBindings(e, mod, &out, x);
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
    computeExportBindings(e, &stmt->mod, &out, x);

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
    if (self->mod.type == ImportDecl) return interpretModImport(n, id, e, x);
    if (self->mod.type == NamespaceDecl) {
      bool hadError = interpretModNamespace(n, id, e, x);
      if (hadError) return resultFlow(FLOW_ERROR, valueNull());
      return resultNormal(valueNull());
    }
    interpretModExport(n, id, e, x);
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
  case NODE_ENUM_DECL:
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
