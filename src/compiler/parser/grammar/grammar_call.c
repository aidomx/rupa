#include <rupa.h>

int grammarParseCallExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a + 1 >= b || t->data[a + 1].type != LPAREN)
    return GRAMMAR_NO_MATCH;

  int c = grammarMatchClose(t, a + 1, b, LPAREN, RPAREN);
  if (c != b - 1)
    return GRAMMAR_NO_MATCH;

  int callee = createId(r->node, t->data[a].value);
  int *as = NULL, n = grammarParseArgs(r, a + 2, c, &as);
  return createCall(r->node, callee, as, n);
}
