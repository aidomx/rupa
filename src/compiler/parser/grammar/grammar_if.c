#include <rupa.h>

/* Parse one conditional arm beginning at `a` and, when present, attach the
 * immediately following elseif/else arm to elseBlock.  The chain is recursive:
 *
 *   if (...)      -> elseBlock = next if
 *   elseif (...)   -> elseBlock = next if
 *   else           -> elseBlock = body block
 */
int grammarParseIf(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (!strcmp(k, "if") || !strcmp(k, "elseif")) {
    int sep = -1;
    for (int i = a + 1; i < b; i++) {
      if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
        sep = i;
        break;
      }
    }

    /* A conditional must own a body delimiter. */
    if (sep < 0)
      return -1;

    int cond = grammarParseExpr(r, a + 1, sep);
    if (cond < 0)
      return -1;

    int bodyStart = (t->data[sep].type == COLON) ? sep + 1 : sep;
    int next = b;
    int body = grammarParseKeywordBody(r, bodyStart, limit, &next);
    if (body < 0)
      return -1;

    int elseBlock = -1;
    int p = next;
    while (p < limit && grammarIsWhitespace(t, p))
      p++;

    if (p < limit && t->data[p].type == KEYWORD) {
      const char *nextKeyword = t->data[p].value;
      if (!strcmp(nextKeyword, "elseif") || !strcmp(nextKeyword, "else")) {
        int end = grammarLineEnd(t, p);
        if (end > limit)
          end = limit;
        int chainNext = p;
        elseBlock = grammarParseIf(r, p, end, limit, &chainNext);
        if (elseBlock < 0)
          return -1;
        next = chainNext;
      }
    }

    *pos = next;
    return createIf(r->node, cond, body, elseBlock);
  }

  if (!strcmp(k, "else")) {
    int sep = -1;
    for (int i = a + 1; i < b; i++) {
      if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
        sep = i;
        break;
      }
    }

    /* else never has a condition. Only ':' or '{' may introduce its body. */
    if (sep < 0)
      return -1;

    int bodyStart = (t->data[sep].type == COLON) ? sep + 1 : sep;
    int next = b;
    int body = grammarParseKeywordBody(r, bodyStart, limit, &next);
    if (body < 0)
      return -1;

    *pos = next;
    return body;
  }

  return GRAMMAR_NO_MATCH;
}
