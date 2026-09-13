#include <rupa.h>

int grammarParseReturnKeyword(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "return"))
    return GRAMMAR_NO_MATCH;

  /* The returned expression may span multiple lines, e.g.
   *   return {
   *     ok: true
   *   }
   * When the expression starts with an opening delimiter, extend b to the
   * matching close token (mirrors grammarParseAssignment). */
  int firstExpr = a + 1;
  while (firstExpr < r->tokens->length && grammarIsWhitespace(t, firstExpr))
    firstExpr++;
  if (firstExpr < r->tokens->length) {
    TokenType ft = t->data[firstExpr].type;
    if (ft == LBRACE || ft == LPAREN || ft == LBLOCK) {
      TokenType close = (ft == LBRACE) ? RBRACE
                      : (ft == LPAREN) ? RPAREN : RBLOCK;
      int end = grammarMatchClose(t, firstExpr, r->tokens->length, ft, close);
      if (end >= 0 && end + 1 > b)
        b = end + 1;
    }
  }

  int e = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return e >= 0 ? createReturn(r->node, e) : -1;
}
