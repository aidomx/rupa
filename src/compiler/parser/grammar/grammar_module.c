#include <rupa.h>

/*
 * Module grammar dispatcher.
 *
 * Delegates to:
 *   grammar_module_import.c  — flat + old-style + bare import parsing
 *   grammar_module_export.c  — export parsing
 *   grammar_module_utils.c   — shared path detection helpers
 *
 * All import/export statements emit NODE_MOD (AstMod) — 1 container for
 * 2 jobs (ImportDecl / ExportDecl). NODE_EXTENDS is reserved for future
 * inheritance (class/prototype warisan) and is untouched.
 */

int grammarParseModule(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "import") && strcmp(k, "export") && strcmp(k, "extends") &&
      strcmp(k, "namespace"))
    return GRAMMAR_NO_MATCH;

  /* Namespace: `namespace db { export ...; export ...; }` — body spans
   * multiple lines/braces, so it needs `limit`, not just line-end `b`. */
  if (!strcmp(k, "namespace")) {
    int id = grammarParseNamespace(r, t, a, limit, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Import: flat syntax, then old style, then bare `import X`
   * (siklus sejajar — design/import_export.txt §Bare). */
  if (!strcmp(k, "import")) {
    int id = grammarParseFlatImport(r, t, a + 1, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;

    id = grammarParseOldImport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;

    id = grammarParseBareImport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Export */
  if (!strcmp(k, "export")) {
    /* Policy block `-> { ... }` boleh multi-line: grammarLineEnd memotong
     * di newline, sehingga isi `{ ... }` di baris berikutnya keluar dari
     * range `b` dan lolos ke parser annotation (simtom: policy hilang,
     * muncul NODE_ANNOTATION hantu). Perluas b ke penutup brace seimbang
     * bila ada ARROW (+ newline opsional) diikuti `{`. */
    for (int i = a; i < b; i++) {
      if (t->data[i].type != ARROW) continue;
      int j = i + 1;
      while (j < limit && (t->data[j].type == NEWLINE || t->data[j].type == TAB)) j++;
      if (j < limit && t->data[j].type == LBRACE) {
        int close = grammarMatchClose(t, j, limit, LBRACE, RBRACE);
        if (close > 0 && close + 1 > b) b = close + 1;
      }
      break;
    }
    int id = grammarParseExport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Extends / fallback */
  int v = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return createModule(r->node, NODE_EXTENDS, v, -1);
}
