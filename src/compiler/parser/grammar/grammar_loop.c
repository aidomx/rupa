#include <rupa.h>

int grammarParseLoop(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "for") && strcmp(k, "rev") && strcmp(k, "while"))
    return GRAMMAR_NO_MATCH;

  int sep = b;
  for (int i = a + 1; i < b; i++)
    if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
      sep = i;
      break;
    }
  int c = grammarParseExpr(r, a + 1, sep);
  int bs = (sep < b && t->data[sep].type == COLON) ? sep + 1 : sep;
  int next = b;
  int body = grammarParseKeywordBody(r, bs, limit, &next);
  *pos = next;
  return createLoop(r->node, k, c, body);
}
