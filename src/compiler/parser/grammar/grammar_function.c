#include <rupa.h>

int grammarParseFunction(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;

  // name(...) { } => function, name(...) => call
  if (t->data[a].type != IDENTIFIER || a + 1 >= b ||
      t->data[a + 1].type != LPAREN)
    return GRAMMAR_NO_MATCH;

  int c = grammarMatchClose(t, a + 1, limit, LPAREN, RPAREN);
  if (c <= 0)
    return GRAMMAR_NO_MATCH;

  int name = parseAtom(r, &t->data[a]);
  int *ps = NULL, n = 0, start = a + 2;
  for (int j = a + 2; j <= c; j++) {
    if (j == c || (j < c && t->data[j].type == COMMA)) {
      int pid = -1;

      /* The lexer normalizes a simple "name: Type" parameter into a
       * single token (IDENTIFIER/LITERAL_ID with .safetyType set), same as
       * it does for "x: Type = value" assignments/annotations. Read that
       * first, the same way grammarParseAnnotation() does; without this,
       * the literal-COLON scan below never finds a COLON token to split
       * on (there isn't one), so the parameter's type was silently
       * dropped. */
      pid = grammarAnnotationFromSafetyType(r, start, -1);
      if (pid < 0) {
        int sep = -1;
        for (int q = start; q < j; q++)
          if (t->data[q].type == COLON) {
            sep = q;
            break;
          }
        if (sep >= 0) {
          int pn = grammarParseExpr(r, start, sep);
          int pt = grammarParseExpr(r, sep + 1, j);
          pid = createAnnotation(r->node, pn, pt, -1);
        } else {
          pid = grammarParseExpr(r, start, j);
        }
      }

      if (pid >= 0)
        grammarPushId(&ps, &n, pid);
      start = j + 1;
    }
  }

  if (c + 1 < limit && t->data[c + 1].type == LBRACE) {
    int close = grammarMatchClose(t, c + 1, limit, LBRACE, RBRACE);
    int body = close >= 0 ? grammarParseBlock(r, c + 1, close) : -1;
    int id = createFunctionDecl(r->node, name, ps, n, body);
    *pos = close >= 0 ? close + 1 : b;
    return id;
  }

  int id = createCall(r->node, name, ps, n);
  *pos = b;
  return id;
}
