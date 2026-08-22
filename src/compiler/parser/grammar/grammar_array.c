#include <rupa.h>

int grammarParseArrayLiteral(Request *r, int a, int b) {
  Token *t = r->tokens;
  int c = grammarMatchClose(t, a, b, LBLOCK, RBLOCK);
  if (c != b - 1)
    return GRAMMAR_NO_MATCH;

  int *es = NULL, n = grammarParseArgs(r, a + 1, c, &es);
  return createArray(r->node, es, n);
}
