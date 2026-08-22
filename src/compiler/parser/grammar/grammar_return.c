#include <rupa.h>

int grammarParseReturnKeyword(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "return"))
    return GRAMMAR_NO_MATCH;

  int e = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return e >= 0 ? createReturn(r->node, e) : -1;
}
