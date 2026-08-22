#include <rupa.h>

int grammarParseBlock(Request *r, int open, int close) {
  int *ids = NULL, n = 0, p = open + 1;
  while (p < close) {
    while (p < close && grammarIsWhitespace(r->tokens, p))
      p++;
    if (p >= close)
      break;
    int id = grammarParseStatement(r, &p, close);
    if (id >= 0)
      grammarPushId(&ids, &n, id);
    else
      p++;
  }
  return createBlock(r->node, ids, n);
}

/* Build a keyword body. A ':' body is exactly one physical line; a '{...}'
 * body owns the complete brace range. This prevents the next top-level
 * statement from being accidentally absorbed as part of a single-line body. */
int grammarParseKeywordBody(Request *r, int bodyStart, int limit, int *next) {
  Token *t = r->tokens;
  if (next)
    *next = bodyStart;
  if (bodyStart >= limit)
    return -1;

  if (t->data[bodyStart].type == LBRACE) {
    int c = grammarMatchClose(t, bodyStart, limit, LBRACE, RBRACE);
    if (c < 0)
      return -1;
    if (next)
      *next = c + 1;
    return grammarParseBlock(r, bodyStart, c);
  }

  int end = grammarLineEnd(t, bodyStart);
  if (end > limit)
    end = limit;
  int p = bodyStart, id = grammarParseStatement(r, &p, end);
  if (id < 0)
    return -1;
  if (next)
    *next = end;
  return createBlock(r->node, &id, 1);
}
