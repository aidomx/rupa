#include <rupa.h>

int grammarParseIf(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (!strcmp(k, "if") || !strcmp(k, "elseif")) {
    int sep = b;
    for (int i = a + 1; i < b; i++)
      if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
        sep = i;
        break;
      }
    int cond = grammarParseExpr(r, a + 1, sep);
    int bodyStart = (sep < b && t->data[sep].type == COLON) ? sep + 1 : sep;
    int next = b;
    int body = grammarParseKeywordBody(r, bodyStart, limit, &next);
    int elseBlock = -1;
    *pos = next;
    return createIf(r->node, cond, body, elseBlock);
  }

  if (!strcmp(k, "else")) {
    int bs = a + 1;
    if (bs < b && t->data[bs].type == LBRACE) {
    } else if (bs < b && t->data[bs].type == COLON)
      bs++;
    int next = b;
    int body = grammarParseKeywordBody(r, bs, limit, &next);
    *pos = next;
    return createIf(r->node, -1, body, -1);
  }

  return GRAMMAR_NO_MATCH;
}
