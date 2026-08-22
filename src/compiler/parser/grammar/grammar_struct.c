#include <rupa.h>

int grammarParseStruct(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;

  // Name { } => struct/blueprint
  if (t->data[a].type != IDENTIFIER || a + 1 >= limit ||
      t->data[a + 1].type != LBRACE)
    return GRAMMAR_NO_MATCH;

  int close = grammarMatchClose(t, a + 1, limit, LBRACE, RBRACE);
  if (close < 0)
    return GRAMMAR_NO_MATCH;

  int name = parseAtom(r, &t->data[a]);
  int body = grammarParseBlock(r, a + 1, close);
  *pos = close + 1;
  return createStructDecl(r->node, name, body);
}
