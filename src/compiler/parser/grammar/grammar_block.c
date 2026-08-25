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
  while (bodyStart < limit && grammarIsWhitespace(t, bodyStart))
    bodyStart++;
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

  /* Some smart-lexer constructs keep a brace handler on one token line even
   * though the original source contains physical newlines. For a ':' body,
   * an elseif/else at the same nesting level is therefore also a reliable
   * hard boundary. */
  if (end == limit) {
    int paren = 0, bracket = 0, brace = 0;
    for (int i = bodyStart; i < limit; i++) {
      TokenType q = t->data[i].type;
      if (q == LPAREN) paren++;
      else if (q == RPAREN && paren > 0) paren--;
      else if (q == LBLOCK) bracket++;
      else if (q == RBLOCK && bracket > 0) bracket--;
      else if (q == LBRACE) brace++;
      else if (q == RBRACE && brace > 0) brace--;
      else if (q == KEYWORD && paren == 0 && bracket == 0 && brace == 0 &&
               (!strcmp(t->data[i].value, "elseif") ||
                !strcmp(t->data[i].value, "else"))) {
        end = i;
        break;
      }
    }
  }

  int p = bodyStart, id = grammarParseStatement(r, &p, end);
  if (id < 0)
    return -1;
  if (next)
    *next = end;
  return createBlock(r->node, &id, 1);
}
