#include <rupa.h>

/* case subject => { pattern: body ... } */
int grammarParseCase(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;
  if (a >= limit || strcmp(t->data[a].value, "case"))
    return GRAMMAR_NO_MATCH;

  int arrow = -1, open = -1;
  for (int i = a + 1; i < limit; i++) {
    if (t->data[i].type == FAT_ARROW) {
      arrow = i;
      continue;
    }
    if (arrow >= 0 && t->data[i].type == LBRACE) {
      open = i;
      break;
    }
    if (arrow < 0 && (t->data[i].type == NEWLINE || isToken(t, i, ENDOF)))
      return -1;
  }
  if (arrow < 0 || open < 0)
    return -1;
  int subject = grammarParseExpr(r, a + 1, arrow);
  if (subject < 0)
    return -1;
  int close = grammarMatchClose(t, open, limit, LBRACE, RBRACE);
  if (close < 0)
    return -1;

  struct AstCaseEntry *entries = NULL;
  int count = 0;
  bool wildcardSeen = false;
  int i = open + 1;
  while (i < close) {
    while (i < close && grammarIsWhitespace(t, i))
      i++;
    if (i >= close)
      break;
    if (wildcardSeen)
      return -1; /* unreachable entry */

    int colon = -1, depth = 0;
    for (int j = i; j < close; j++) {
      TokenType ty = t->data[j].type;
      if (ty == LPAREN || ty == LBLOCK || ty == LBRACE)
        depth++;
      else if (ty == RPAREN || ty == RBLOCK || ty == RBRACE)
        depth--;
      else if (ty == COLON && depth == 0) {
        colon = j;
        break;
      }
    }
    if (colon < 0)
      return -1;

    struct AstCaseEntry e = {.pattern = -1, .body = -1, .wildcard = false};
    if (colon == i + 1 &&
        ((t->data[i].type == KEYWORD || t->data[i].type == LITERAL_ID ||
          t->data[i].type == IDENTIFIER) &&
         !strcmp(t->data[i].value, "default"))) {
      e.wildcard = true;
      wildcardSeen = true;
    } else {
      e.pattern = grammarParseExpr(r, i, colon);
      if (e.pattern < 0)
        return -1;
    }

    int bodyStart = colon + 1;
    while (bodyStart < close && grammarIsWhitespace(t, bodyStart))
      bodyStart++;
    int next = bodyStart;
    if (bodyStart < close && t->data[bodyStart].type == LBRACE) {
      int bodyClose = grammarMatchClose(t, bodyStart, close, LBRACE, RBRACE);
      if (bodyClose < 0)
        return -1;
      e.body = grammarParseBlock(r, bodyStart, bodyClose);
      if (e.body < 0)
        return -1;
      next = bodyClose + 1;
    } else {
      /* The smart lexer may omit NEWLINE tokens and assign the same line
       * number to every token inside case braces, so grammarLineEnd is
       * useless here.  Instead, scan forward structurally for the next
       * COLON at depth 0 — that marks the start of the next entry.
       * Everything in between is this entry's body. */
      int bodyEnd = close;
      int d = 0;
      for (int j = colon + 1; j < close; j++) {
        TokenType ty = t->data[j].type;
        if (ty == LPAREN || ty == LBLOCK || ty == LBRACE)
          d++;
        else if (ty == RPAREN || ty == RBLOCK || ty == RBRACE)
          d--;
        else if (ty == COLON && d == 0) {
          bodyEnd = j;
          break;
        }
      }
      e.body = grammarParseStatement(r, &next, bodyEnd);
      /* Advance next past the body.  When bodyEnd is an interior colon
       * (next entry delimiter), back up one token so the outer loop can
       * re-find that colon.  When bodyEnd == close there is nothing
       * more to parse. */
      if (bodyEnd < close)
        next = bodyEnd - 1;
      else
        next = bodyEnd;
    }
    if (e.body < 0)
      return -1;
    entries = gcrealloc(entries, sizeof(*entries) * (count + 1));
    entries[count++] = e;
    i = next;
  }
  *pos = close + 1;
  return createCase(r->node, subject, entries, count);
}
