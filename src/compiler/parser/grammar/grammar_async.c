#include <rupa.h>

static int findTopLevel(Token *t, int a, int b, TokenType type) {
  int paren = 0, block = 0, brace = 0;
  for (int i = a; i < b; i++) {
    TokenType q = t->data[i].type;
    if (q == LPAREN) paren++;
    else if (q == RPAREN) paren--;
    else if (q == LBLOCK) block++;
    else if (q == RBLOCK) block--;
    else if (q == LBRACE) brace++;
    else if (q == RBRACE) brace--;
    else if (q == type && paren == 0 && block == 0 && brace == 0)
      return i;
  }
  return -1;
}

int grammarParseAsyncExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a >= b || t->data[a].type != KEYWORD || strcmp(t->data[a].value, "async"))
    return GRAMMAR_NO_MATCH;

  int fat = findTopLevel(t, a + 1, b, FAT_ARROW);
  int timeout = findTopLevel(t, a + 1, b, ARROW);
  if (timeout >= 0 && fat >= 0 && timeout < fat)
    timeout = -1;

  int requestEnd = b;
  if (fat >= 0) requestEnd = fat;
  else if (timeout >= 0) requestEnd = timeout;
  int request = grammarParseExpr(r, a + 1, requestEnd);
  if (request < 0) return -1;

  int handler = -1;
  if (fat >= 0) {
    int hs = fat + 1;
    int he = timeout >= 0 ? timeout : b;
    while (hs < he && grammarIsWhitespace(t, hs)) hs++;
    while (he > hs && grammarIsWhitespace(t, he - 1)) he--;
    if (hs >= he) return -1;
    if (t->data[hs].type == LBRACE) {
      int close = grammarMatchClose(t, hs, he, LBRACE, RBRACE);
      if (close != he - 1) return -1;
      handler = grammarParseBlock(r, hs, close);
    } else if (t->data[hs].type == KEYWORD) {
      int hp = hs;
      handler = grammarParseStatement(r, &hp, he);
      if (hp != he) return -1;
    } else {
      handler = grammarParseExpr(r, hs, he);
    }
    if (handler < 0) return -1;
  }

  int timeoutId = -1;
  if (timeout >= 0) {
    timeoutId = grammarParseExpr(r, timeout + 1, b);
    if (timeoutId < 0) return -1;
  }
  return createAsync(r->node, request, handler, timeoutId);
}

int grammarParseAwaitExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a >= b || t->data[a].type != KEYWORD || strcmp(t->data[a].value, "await"))
    return GRAMMAR_NO_MATCH;

  /* await is a prefix expression. Stop its operand at the first top-level
   * binary operator so `await value != null` means `(await value) != null`. */
  int paren = 0, block = 0, brace = 0, split = -1;
  for (int i = a + 1; i < b; i++) {
    TokenType q = t->data[i].type;
    if (q == LPAREN) paren++;
    else if (q == RPAREN) paren--;
    else if (q == LBLOCK) block++;
    else if (q == RBLOCK) block--;
    else if (q == LBRACE) brace++;
    else if (q == RBRACE) brace--;
    else if (paren == 0 && block == 0 && brace == 0 &&
             getPrecedence(&t->data[i]) >= 0) {
      split = i;
      break;
    }
  }

  int operandEnd = split >= 0 ? split : b;
  int operand = grammarParseExpr(r, a + 1, operandEnd);
  if (operand < 0) return -1;
  int awaited = createAwait(r->node, operand);
  if (awaited < 0 || split < 0) return awaited;

  int right = grammarParseExpr(r, split + 1, b);
  if (right < 0) return -1;
  return createBinary(r->node, &t->data[split], awaited, right);
}
