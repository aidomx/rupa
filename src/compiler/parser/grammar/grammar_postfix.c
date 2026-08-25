#include <rupa.h>

/* Parse a primary followed by postfix operations: .member, (...), [...]. */
int grammarParsePostfixExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  while (a < b && grammarIsWhitespace(t, a)) a++;
  while (b > a && grammarIsWhitespace(t, b - 1)) b--;
  if (a >= b || (t->data[a].type != IDENTIFIER && t->data[a].type != LITERAL_ID))
    return GRAMMAR_NO_MATCH;

  int current = createId(r->node, t->data[a].value);
  if (current < 0) return -1;
  int i = a + 1;
  bool postfix = false;

  while (i < b) {
    if (t->data[i].type == DOT) {
      if (i + 1 >= b || (t->data[i + 1].type != IDENTIFIER && t->data[i + 1].type != LITERAL_ID))
        return -1;
      int member = createId(r->node, t->data[i + 1].value);
      current = createMember(r->node, current, member);
      if (current < 0) return -1;
      i += 2; postfix = true; continue;
    }
    if (t->data[i].type == LPAREN) {
      int close = grammarMatchClose(t, i, b, LPAREN, RPAREN);
      if (close < 0) return -1;
      int *args = NULL, n = grammarParseArgs(r, i + 1, close, &args);
      current = createCall(r->node, current, args, n);
      if (current < 0) return -1;
      i = close + 1; postfix = true; continue;
    }
    if (t->data[i].type == LBLOCK) {
      int close = grammarMatchClose(t, i, b, LBLOCK, RBLOCK);
      if (close < 0) return -1;
      int index = close == i + 1 ? -1 : grammarParseExpr(r, i + 1, close);
      current = createSubscript(r->node, current, index);
      if (current < 0) return -1;
      i = close + 1; postfix = true; continue;
    }
    return GRAMMAR_NO_MATCH;
  }
  return postfix ? current : GRAMMAR_NO_MATCH;
}
