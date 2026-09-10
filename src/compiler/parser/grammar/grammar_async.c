#include <rupa.h>

static int findTopLevel(Token *t, int a, int b, TokenType type) {
  int paren = 0, block = 0, brace = 0;
  for (int i = a; i < b; i++) {
    TokenType q = t->data[i].type;
    if (q == LPAREN)
      paren++;
    else if (q == RPAREN)
      paren--;
    else if (q == LBLOCK)
      block++;
    else if (q == RBLOCK)
      block--;
    else if (q == LBRACE)
      brace++;
    else if (q == RBRACE)
      brace--;
    else if (q == type && paren == 0 && block == 0 && brace == 0)
      return i;
  }
  return -1;
}

int grammarParseAsyncExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a >= b || t->data[a].type != KEYWORD || strcmp(t->data[a].value, "async"))
    return GRAMMAR_NO_MATCH;

  /* Find "->" (then) after request expression */
  int arrow = findTopLevel(t, a + 1, b, ARROW);

  int requestEnd = arrow >= 0 ? arrow : b;
  int request = grammarParseExpr(r, a + 1, requestEnd);
  if (request < 0) return -1;

  int loaderId = -1;
  int timeoutId = -1;

  /* `async request -> {loader, timeout}` — satu-satunya bentuk `->` yang
   * valid untuk async. `->` di sini adalah grammar "then" biasa (bukan
   * block handler); isi `{}` SELALU pasangan (loader, timeout), dua slot
   * dipisah koma. loader = referensi ke function yang sudah dideklarasikan
   * terpisah; timeout = angka literal langsung atau referensi identifier
   * ke variabel yang sudah didefinisikan. `->` boleh tidak ada sama sekali
   * (mis. `users = async db.getUser()`), keduanya lalu tetap -1. */
  if (arrow >= 0) {
    int hs = arrow + 1;
    int he = b;
    while (hs < he && grammarIsWhitespace(t, hs))
      hs++;
    while (he > hs && grammarIsWhitespace(t, he - 1))
      he--;
    if (hs >= he || t->data[hs].type != LBRACE)
      return -1;

    int close = grammarMatchClose(t, hs, he, LBRACE, RBRACE);
    if (close < 0) return -1;

    int p = hs + 1;
    while (p < close && grammarIsWhitespace(t, p)) p++;
    if (p >= close || (t->data[p].type != IDENTIFIER && t->data[p].type != LITERAL_ID))
      return -1;
    int id1 = p;
    p++;
    while (p < close && grammarIsWhitespace(t, p)) p++;
    if (p >= close || t->data[p].type != COMMA)
      return -1;
    p++;
    while (p < close && grammarIsWhitespace(t, p)) p++;
    if (p >= close || (t->data[p].type != IDENTIFIER && t->data[p].type != LITERAL_ID &&
                       t->data[p].type != NUMBER))
      return -1;
    int id2 = p;
    p++;
    while (p < close && grammarIsWhitespace(t, p)) p++;
    if (p != close) return -1;

    loaderId = createId(r->node, t->data[id1].value);
    /* Timeout: angka literal langsung, atau referensi identifier */
    if (t->data[id2].type == NUMBER)
      timeoutId = createNumber(r->node, atoi(t->data[id2].value));
    else
      timeoutId = createId(r->node, t->data[id2].value);
    if (loaderId < 0 || timeoutId < 0) return -1;
  }

  return createAsync(r->node, request, -1, -1, loaderId, timeoutId);
}

int grammarParseAwaitExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a >= b || t->data[a].type != KEYWORD || strcmp(t->data[a].value, "await"))
    return GRAMMAR_NO_MATCH;

  /* await is a prefix expression. Stop its operand at the first top-level
   * binary operator or member access (.) so that:
   *   `await value != null` means `(await value) != null`
   *   `await users.data` means `(await users).data` */
  int paren = 0, block = 0, brace = 0, split = -1;
  for (int i = a + 1; i < b; i++) {
    TokenType q = t->data[i].type;
    if (q == LPAREN)
      paren++;
    else if (q == RPAREN)
      paren--;
    else if (q == LBLOCK)
      block++;
    else if (q == RBLOCK)
      block--;
    else if (q == LBRACE)
      brace++;
    else if (q == RBRACE)
      brace--;
    else if (paren == 0 && block == 0 && brace == 0 &&
             (q == DOT || getPrecedence(&t->data[i]) >= 0)) {
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
