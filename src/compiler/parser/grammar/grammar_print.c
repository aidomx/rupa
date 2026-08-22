#include <rupa.h>

int grammarParsePrint(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "print"))
    return GRAMMAR_NO_MATCH;

  int o = a + 1;
  int c = (o < b && t->data[o].type == LPAREN)
              ? grammarMatchClose(t, o, b, LPAREN, RPAREN)
              : -1;
  int *as = NULL, n = (c >= 0) ? grammarParseArgs(r, o + 1, c, &as) : 0;
  int id = createPrint(r->node, as, n);
  *pos = b;
  return id;
}
