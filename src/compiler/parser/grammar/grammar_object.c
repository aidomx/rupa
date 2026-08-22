#include <rupa.h>

int grammarParseObjectLiteral(Request *r, int a, int b) {
  Token *t = r->tokens;
  int c = grammarMatchClose(t, a, b, LBRACE, RBRACE);
  if (c != b - 1)
    return GRAMMAR_NO_MATCH;

  struct AstObjectEntry *entries = NULL;
  int n = 0, start = a + 1, d = 0;
  for (int i = a + 1; i <= c; i++) {
    if (i < c) {
      TokenType q = t->data[i].type;
      if (q == LPAREN || q == LBLOCK || q == LBRACE)
        d++;
      else if (q == RPAREN || q == RBLOCK || q == RBRACE)
        d--;
    }
    if (i == c || (i < c && t->data[i].type == COMMA && d == 0)) {
      int sep = -1, depth = 0;
      for (int j = start; j < i; j++) {
        TokenType q = t->data[j].type;
        if (q == LPAREN || q == LBLOCK || q == LBRACE)
          depth++;
        else if (q == RPAREN || q == RBLOCK || q == RBRACE)
          depth--;
        else if (q == COLON && depth == 0) {
          sep = j;
          break;
        }
      }
      if (sep >= 0) {
        int key = grammarParseExpr(r, start, sep);
        int value = grammarParseExpr(r, sep + 1, i);
        if (key >= 0 && value >= 0) {
          entries = gcrealloc(entries, sizeof(*entries) * (n + 1));
          entries[n++] = (struct AstObjectEntry){key, value};
        }
      }
      start = i + 1;
    }
  }
  return createObject(r->node, entries, n);
}
