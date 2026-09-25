#include <rupa.h>

/*
 * Export grammar — emits NODE_MOD (AstMod, ExportDecl) for the canonical
 * forms (design/import_export.txt):
 *
 *   export * from ./X                    (flatten seluruh member X)
 *   export a, b from ./c                 (selective re-export)
 *   export c from ./c                    (re-export leaf/sub-package)
 *   export c from ./c -> { a: private }  (dengan policy)
 *   export open from ./open              (member-first: fungsi `open`
 *                                         menang atas whole module)
 *
 * Plus the block form, which merges several of the above under one name:
 *
 *   namespace db {
 *     export driver from ./driver
 *     export table from ./table -> { rawQuery: private }
 *   }
 *
 * Bare export (tanpa `from`) ada untuk siklus sejajar — member env
 * sendiri atau file sejajar bernama sama (design §Bare + §Kelemahan). */

/* Parse policy entries inside `-> { name: private/public, ... }`.
 * Values other than private/public are still captured as-is. */
static int parseExportPolicies(Token *t, int policyStart, int b, AstModEntry **policies) {
  int count = 0;
  if (policyStart < b && t->data[policyStart].type == ARROW) {
    policyStart++;
    if (policyStart < b && t->data[policyStart].type == LBRACE) {
      policyStart++;
      while (policyStart < b && t->data[policyStart].type != RBRACE) {
        if (t->data[policyStart].type == COMMA) {
          policyStart++;
          continue;
        }

        /* Nama policy boleh keyword juga: `-> { import: public }`
         * (opt-in ekspos binding hasil import di whole-env export). */
        if (t->data[policyStart].type != IDENTIFIER &&
            t->data[policyStart].type != LITERAL_ID && t->data[policyStart].type != KEYWORD)
          break;

        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s", t->data[policyStart].value);
        policyStart++;

        if (policyStart >= b || t->data[policyStart].type != COLON)
          break;
        policyStart++;

        if (policyStart >= b ||
            (t->data[policyStart].type != KEYWORD && t->data[policyStart].type != IDENTIFIER))
          break;

        policies[count] = modPolicy(nameBuf, t->data[policyStart].value);
        count++;
        policyStart++;
      }
      if (policyStart < b && t->data[policyStart].type == RBRACE)
        policyStart++;
    }
  }
  return count;
}

/* Parse export entries between baseIdx and limit: dotted paths
 * (`driver.connect`), wildcards (`driver.*`), and `as alias`. Mirrors
 * parseFlatImportEntries in grammar_module_import.c so both sides of the
 * language treat `x.y.z [as w]` identically. Stops at `from` or `->`,
 * leaving those to the caller. */
static int parseExportEntries(Token *t, int baseIdx, int limit, AstModEntry **entries,
                              int *entryCount) {
  int cur = baseIdx;
  *entryCount = 0;

  while (cur < limit) {
    /* Statement berakhir di akhir baris — export tidak boleh menelan
     * statement baris berikutnya. */
    if (t->data[cur].type == NEWLINE || t->data[cur].type == ENDOF) break;
    if (t->data[cur].type == COMMA) {
      cur++;
      continue;
    }

    /* Leading wildcard: `export *` (self, ie.txt #1) atau
     * `export * from ./X` — flatten seluruh member. Entry bernama
     * kosong; `*` selalu entry terakhir (bentuk lain bukan grammar). */
    if (t->data[cur].type == STAR) {
      AstModEntry *e = modEntry("");
      if (!e) break;
      e->type = MOD_WILD;
      entries[(*entryCount)++] = e;
      cur++;
      break;
    }

    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "from"))
      break;
    if (t->data[cur].type == ARROW) break;

    if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID) break;

    /* Nama polos saja — member chain (x.y), wildcard member (x.*), dan
     * entry alias (x as y) dihapus dari grammar (bentuk warisan).
     * Selective export = daftar nama polos. */
    entries[(*entryCount)++] = modEntry(t->data[cur].value);
    cur++;
  }
  return cur;
}

/* Handle export: `export * from ./X`, `export a, b from ./c`,
 * `export c from ./c -> {...}`, plus bare local `export x` / `export x, y`
 * (siklus sejajar, design/import_export.txt §Bare). */
int grammarParseExport(Request *r, Token *t, int a, int b, int *pos) {
  int cur = a + 1;

  AstModEntry *entries[64];
  int entryCount = 0;
  cur = parseExportEntries(t, cur, b, entries, &entryCount);

  int fromPos = -1;
  if (cur < b &&
      (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
       t->data[cur].type == LITERAL_ID) &&
      !strcmp(t->data[cur].value, "from"))
    fromPos = cur;

  if (entryCount == 0 || fromPos < 0) {
    /* Bare local export (design/import_export.txt §Bare) — siklus
     * sejajar: `export x, y` (member sendiri) atau `export X`
     * (X.rp sejajar). Runtime yang menegakkan batasannya. Policy
     * lokal `-> {...}` tetap diterima. */
    AstModEntry *policies[64];
    int policyCount = parseExportPolicies(t, cur, b, policies);

    int end = cur;
    if (policyCount > 0) {
      int scan = cur;
      if (scan < b && t->data[scan].type == ARROW) {
        scan++;
        if (scan < b && t->data[scan].type == LBRACE) {
          scan++;
          while (scan < b && t->data[scan].type != RBRACE) scan++;
          if (scan < b) scan++;
        }
        end = scan;
      }
    }

    *pos = (end > cur) ? end : cur;
    return createModExport(r->node, entries, entryCount, NULL, policies, policyCount);
  }

  /* Parse 'from PATH' */
  int fromPathEnd = -1;
  int fromStart = grammarModuleDetectFrom(t, fromPos, b, &fromPathEnd);
  if (fromStart < 0) {
    *pos = b;
    return GRAMMAR_NO_MATCH;
  }

  char *fromPath = grammarModuleBuildPath(t, fromStart, fromPathEnd);

  /* Parse optional '-> { ... }' policy block */
  AstModEntry *policies[64];
  int policyCount = parseExportPolicies(t, fromPathEnd + 1, b, policies);

  /* Compute end position: past policy block if present */
  int policyEnd = fromPathEnd + 1;
  if (policyCount > 0) {
    int scan = fromPathEnd + 1;
    if (scan < b && t->data[scan].type == ARROW) {
      scan++;
      if (scan < b && t->data[scan].type == LBRACE) {
        scan++;
        while (scan < b && t->data[scan].type != RBRACE)
          scan++;
        if (scan < b)
          scan++;
      }
      policyEnd = scan;
    }
  }

  *pos = policyEnd;
  int id = createModExport(r->node, entries, entryCount, fromPath, policies, policyCount);
  free(fromPath);
  return id;
}

/* Handle `namespace name { export ...; export ...; }`.
 * The body's statements are parsed by the ordinary statement grammar
 * (grammarParseKeywordBody → grammarParseBlock → grammarParseStatement) —
 * each line inside is just a normal `export ...` statement. The interpreter
 * (dispatch.c) walks the resulting NODE_BLOCK, evaluates each ExportDecl,
 * and merges the results under `name` instead of binding them to env
 * directly, erroring on duplicate bind-names within the block. */
int grammarParseNamespace(Request *r, Token *t, int a, int limit, int *pos) {
  int cur = a + 1;
  if (cur >= limit || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
    return GRAMMAR_NO_MATCH;

  char nameBuf[128];
  snprintf(nameBuf, sizeof(nameBuf), "%s", t->data[cur].value);
  cur++;

  while (cur < limit && grammarIsWhitespace(t, cur)) cur++;
  if (cur >= limit || t->data[cur].type != LBRACE) return GRAMMAR_NO_MATCH;

  int next = cur;
  int body = grammarParseKeywordBody(r, cur, limit, &next);
  if (body < 0) return GRAMMAR_NO_MATCH;

  *pos = next;
  return createModNamespace(r->node, nameBuf, body);
}
