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

  /* Import: flat syntax, then old style, then bare `import X` */
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
    int id = grammarParseExport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Extends / fallback */
  int v = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return createModule(r->node, NODE_EXTENDS, v, -1);
}
