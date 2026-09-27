#include <rupa.h>

int grammarParseLoop(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "for") && strcmp(k, "rev") && strcmp(k, "while"))
    return GRAMMAR_NO_MATCH;

  /* old_loop/mixed (design/next_loop.txt): `for i=0; i < 10: ...` /
   * `rev i=10; i > 0 { ... }` — header berisi `init; cond` dipisah
   * SEMICOLON. Increment/decrement diurus sistem (for maju, rev
   * mundur) — tidak ada bagian ketiga. */
  int semi = -1;
  for (int i = a + 1; i < b; i++) {
    if (t->data[i].type == SEMICOLON) {
      semi = i;
      break;
    }
  }

  if (semi > a + 1 && semi < b) {
    int sep = b;
    for (int i = semi + 1; i < b; i++)
      if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
        sep = i;
        break;
      }
    if (sep > semi + 1) {
      /* Init diparse sebagai statement assignment sungguhan
       * (NODE_ASSIGN — sama dengan `i = 0` statement biasa), bukan
       * expression: `=` di Rupa bukan expression, jadi grammarParseExpr
       * akan menghasilkan Binary("=") yang tidak menulis variable. */
      int ptmp = semi;
      int init = grammarParseAssignment(r, a + 1, semi, &ptmp);
      int cond = grammarParseExpr(r, semi + 1, sep);
      if (init < 0 || init == GRAMMAR_NO_MATCH || cond < 0)
        return GRAMMAR_NO_MATCH;
      int bs = (sep < b && t->data[sep].type == COLON) ? sep + 1 : sep;
      int next = b;
      int body = grammarParseKeywordBody(r, bs, limit, &next);
      *pos = next;
      return createLoopInit(r->node, k, init, cond, body);
    }
    /* `init;` tanpa cond → biarkan jalur umum (semicolon bukan bagian
     * header loop yang sah di sini). */
  }

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
