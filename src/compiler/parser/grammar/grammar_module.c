#include <rupa.h>

int grammarParseModule(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "import") && strcmp(k, "export") && strcmp(k, "extends"))
    return GRAMMAR_NO_MATCH;

  int v = grammarParseExpr(r, a + 1, b);
  NodeType nt = !strcmp(k, "import")
                    ? NODE_IMPORT
                    : (!strcmp(k, "export") ? NODE_EXPORT : NODE_EXTENDS);
  *pos = b;
  return createModule(r->node, nt, v);
}
