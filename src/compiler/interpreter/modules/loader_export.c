#include <rupa.h>
/* setelah rupa.h: guard-nya bergantung RUPA_PACKAGE_H */
#include "module.h"

/* loader_export.c — pembangunan object hasil export module: bare
 * re-export, `export * from ./X`, namespace re-export, selective
 * member, dengan policy private (null + metadata `_private`).
 * Unit hasil split loader.c:
 *   loader_state.c   — state global + module cache
 *   loader_path.c    — resolusi path import
 *   loader_export.c  — export entries (file ini)
 *   loader.c         — entry: loadModuleFile/loadModuleFileError
 */

/* Bangun object entries dari semua NODE_MOD ExportDecl di program root.
 * `module_path` = path import asal (untuk bare re-export "./name").
 * Member whole module diambil via loadModuleFileError (rekursi loader).
 * Return VALUE_OBJECT berisi entries; pemanggil yang memutuskan cache. */
RuntimeValue modBuildExportEntries(Node *node, int root, const char *module_path, Error *error) {
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
  return valueObject(entries);
}
