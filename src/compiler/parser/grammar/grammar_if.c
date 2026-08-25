#include <rupa.h>

/* Parse if / elseif / else as one recursive chain.
 *
 * Each ':' arm owns one physical statement line. A '{...}' arm owns its
 * complete brace range. After an arm is consumed, the next keyword is checked
 * at the same parser level and attached as the else branch.
 */
int grammarParseIf(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;
  if (a >= limit || t->data[a].type != KEYWORD)
    return GRAMMAR_NO_MATCH;

  const char *k = t->data[a].value;
  bool conditional = !strcmp(k, "if") || !strcmp(k, "elseif");
  bool otherwise = !strcmp(k, "else");
  if (!conditional && !otherwise)
    return GRAMMAR_NO_MATCH;

  int sep = -1;
  for (int i = a + 1; i < b; i++) {
    if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
      sep = i;
      break;
    }
  }
  if (sep < 0)
    return -1;

  int condition = -1;
  if (conditional) {
    condition = grammarParseExpr(r, a + 1, sep);
    if (condition < 0)
      return -1;
  }

  int bodyStart = t->data[sep].type == COLON ? sep + 1 : sep;
  int next = bodyStart;
  int body = grammarParseKeywordBody(r, bodyStart, limit, &next);
  if (body < 0)
    return -1;

  /* Do not depend on the exact newline token position returned by the body. */
  int p = next;
  while (p < limit && grammarIsWhitespace(t, p))
    p++;

  if (otherwise) {
    *pos = next;
    return body;
  }

  int elseBlock = -1;
  if (p < limit && t->data[p].type == KEYWORD &&
      (!strcmp(t->data[p].value, "elseif") || !strcmp(t->data[p].value, "else"))) {
    int armEnd = grammarLineEnd(t, p);
    if (armEnd > limit)
      armEnd = limit;
    int chainPos = p;
    elseBlock = grammarParseIf(r, p, armEnd, limit, &chainPos);
    if (elseBlock < 0)
      return -1;
    next = chainPos;
  }

  *pos = next;
  return createIf(r->node, condition, body, elseBlock);
}
