#include <rupa.h>

/* case subject => { pattern: body ... } */
int grammarParseCase(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;
  if (a >= limit || strcmp(t->data[a].value, "case"))
    return GRAMMAR_NO_MATCH;

  int arrow = -1, open = -1;
  for (int i = a + 1; i < limit; i++) {
    if (t->data[i].type == FAT_ARROW) { arrow = i; continue; }
    if (arrow >= 0 && t->data[i].type == LBRACE) { open = i; break; }
    if (arrow < 0 && (t->data[i].type == NEWLINE || isToken(t, i, ENDOF))) return -1;
  }
  if (arrow < 0 || open < 0) return -1;
  int subject = grammarParseExpr(r, a + 1, arrow);
  if (subject < 0) return -1;
  int close = grammarMatchClose(t, open, limit, LBRACE, RBRACE);
  if (close < 0) return -1;

  struct AstCaseEntry *entries = NULL; int count = 0; bool wildcardSeen = false;
  int i = open + 1;
  while (i < close) {
    while (i < close && grammarIsWhitespace(t, i)) i++;
    if (i >= close) break;
    if (wildcardSeen) return -1; /* unreachable entry */

    int colon = -1, depth = 0;
    for (int j = i; j < close; j++) {
      TokenType ty = t->data[j].type;
      if (ty == LPAREN || ty == LBLOCK || ty == LBRACE) depth++;
      else if (ty == RPAREN || ty == RBLOCK || ty == RBRACE) depth--;
      else if (ty == COLON && depth == 0) { colon = j; break; }
    }
    if (colon < 0) return -1;

    struct AstCaseEntry e = {.pattern = -1, .body = -1, .wildcard = false};
    if (colon == i + 1 && t->data[i].type == STAR) {
      e.wildcard = true; wildcardSeen = true;
    } else {
      e.pattern = grammarParseExpr(r, i, colon);
      if (e.pattern < 0) return -1;
    }

    int bodyStart = colon + 1;
    while (bodyStart < close && grammarIsWhitespace(t, bodyStart)) bodyStart++;
    int next = bodyStart;
    if (bodyStart < close && t->data[bodyStart].type == LBRACE) {
      int bodyClose = grammarMatchClose(t, bodyStart, close, LBRACE, RBRACE);
      if (bodyClose < 0) return -1;
      /* Case blocks need structural statement boundaries because the smart
       * lexer can omit NEWLINE tokens inside braces. */
      int *items = NULL, itemCount = 0, q = bodyStart + 1;
      while (q < bodyClose) {
        while (q < bodyClose && grammarIsWhitespace(t, q)) q++;
        if (q >= bodyClose) break;
        int end = q + 1;
        if (q + 1 < bodyClose && t->data[q + 1].type == LPAREN) {
          int c = grammarMatchClose(t, q + 1, bodyClose, LPAREN, RPAREN);
          if (c < 0) return -1;
          end = c + 1;
        }
        int stmtPos = q;
        int stmt = grammarParseStatement(r, &stmtPos, end);
        if (stmt < 0) return -1;
        grammarPushId(&items, &itemCount, stmt);
        q = end;
      }
      e.body = createBlock(r->node, items, itemCount);
      next = bodyClose + 1;
    } else {
      /* The smart lexer may omit NEWLINE tokens inside case braces. Determine
       * the end of common inline bodies structurally, so the next pattern is
       * not swallowed by the current entry. */
      int bodyEnd = bodyStart + 1;
      if (t->data[bodyStart].type == KEYWORD &&
          !strcmp(t->data[bodyStart].value, "print") &&
          bodyStart + 1 < close && t->data[bodyStart + 1].type == LPAREN) {
        int c = grammarMatchClose(t, bodyStart + 1, close, LPAREN, RPAREN);
        bodyEnd = c >= 0 ? c + 1 : -1;
      } else if (bodyStart + 1 < close && t->data[bodyStart + 1].type == LPAREN) {
        int c = grammarMatchClose(t, bodyStart + 1, close, LPAREN, RPAREN);
        bodyEnd = c >= 0 ? c + 1 : -1;
      }
      if (bodyEnd < 0) return -1;
      e.body = grammarParseStatement(r, &next, bodyEnd);
      next = bodyEnd;
    }
    if (e.body < 0) return -1;
    entries = gcrealloc(entries, sizeof(*entries) * (count + 1));
    entries[count++] = e;
    i = next;
  }
  *pos = close + 1;
  return createCase(r->node, subject, entries, count);
}
