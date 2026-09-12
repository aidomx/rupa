#include <rupa.h>

/*
 * Module grammar dispatcher.
 *
 * Delegates to:
 *   grammar_module_import.c  — flat + old-style import parsing
 *   grammar_module_export.c  — export parsing
 *   grammar_module_utils.c   — shared path detection helpers
 */

int grammarParseModule(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "import") && strcmp(k, "export") && strcmp(k, "extends")) return GRAMMAR_NO_MATCH;

  NodeType nt =
      !strcmp(k, "import") ? NODE_IMPORT : (!strcmp(k, "export") ? NODE_EXPORT : NODE_EXTENDS);

  /* Import: try flat syntax first, then old style */
  if (nt == NODE_IMPORT) {
    int id = grammarParseFlatImport(r, t, a + 1, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;

    id = grammarParseOldImport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Export */
  if (nt == NODE_EXPORT) {
    int id = grammarParseExport(r, t, a, b, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  /* Extends / fallback */
  int v = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return createModule(r->node, nt, v, -1);
}
