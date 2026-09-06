#include <rupa.h>

/**
 * Returns true when `type` is a token that can appear on the right side of
 * a binary operator (i.e. an operand).  Used to distinguish unary minus
 * from binary minus: `-` after an operand is binary, otherwise unary. */
static bool isOperand(TokenType type) {
  switch (type) {
  case IDENTIFIER:
  case LITERAL_ID:
  case NUMBER:
  case DECIMAL:
  case BOOLEAN:
  case STRING:
  case RPAREN:
  case RBLOCK:
  case RBRACE:
    return true;
  default:
    return false;
  }
}

/**
 * parseBinary: parser rekursif untuk binary expression.
 * - Menjaga precedence.
 * - Mengabaikan operator dalam tanda kurung.
 */
int parseBinary(Request *req, int start, int end) {
  if (!req || start >= end)
    return -1;
  Token *tokens = req->tokens;

  int minPrec = 0x3f3f3f3f;
  int minIndex = -1;
  int depth = 0;

  // Skip whitespace at beginning
  while (start < end &&
         (isToken(tokens, start, NEWLINE) || isToken(tokens, start, TAB))) {
    start++;
  }

  // Skip whitespace at end
  while (end > start &&
         (isToken(tokens, end - 1, NEWLINE) || isToken(tokens, end - 1, TAB))) {
    end--;
  }

  if (start >= end)
    return -1; // Empty after trimming

  /* Prefix expression grammar must also be visible to recursive binary parsing.
   * Without this, the right side of `i < await users.data.length` reaches
   * parseAtom() and `await` is treated as a plain identifier. */
  if (tokens->data[start].type == KEYWORD) {
    int id = grammarParseAsyncExpr(req, start, end);
    if (id != GRAMMAR_NO_MATCH)
      return id;
    id = grammarParseAwaitExpr(req, start, end);
    if (id != GRAMMAR_NO_MATCH)
      return id;
  }

  // cari operator top-level (depth == 0)
  for (int i = start; i < end; i++) {
    if (isToken(tokens, i, LPAREN)) {
      depth++;
      continue;
    }
    if (isToken(tokens, i, RPAREN)) {
      depth--;
      continue;
    }

    if (isToken(tokens, i, LBLOCK)) {
      int rpos = findArr(tokens, i);
      if (rpos == -1)
        break;
      i = rpos;
      continue;
    }

    if (isToken(tokens, i, RBLOCK)) {
      depth = (depth > 0) ? depth - 1 : 0;
      continue;
    }

    if (depth > 0)
      continue;

    int prec = getPrecedence(&tokens->data[i]);
    if (prec >= 0 && prec <= minPrec) {
      /* A MINUS that is NOT preceded by an operand is a unary minus
       * (e.g. `-1`, `x * -1`).  Treat it as part of the operand, not
       * a binary split point. */
      if (tokens->data[i].type == MINUS &&
          (i == start || !isOperand(tokens->data[i - 1].type)))
        continue;
      minPrec = prec;
      minIndex = i;
    }
  }

  // tidak ada operator di level atas
  if (minIndex == -1) {
    /* Unary minus: `-expr` → `0 - expr` */
    if (isToken(tokens, start, MINUS)) {
      int operand = parseBinary(req, start + 1, end);
      if (operand >= 0) {
        int zero = createNumber(req->node, 0);
        return createBinary(req->node, &tokens->data[start], zero, operand);
      }
    }

    if (isToken(tokens, start, LPAREN)) {
      int k = findParen(tokens, start, end);
      if (k == end - 1) {
        // kupas kurung luar
        return parseBinary(req, start + 1, k);
      }
    }

    else if (isToken(tokens, start, LBLOCK)) {
      int k = findArr(tokens, start);
      if (k == end - 1) {
        return parseBinary(req, start + 1, k);
      }
    }

    int postfix = grammarParsePostfixExpr(req, start, end);
    if (postfix != GRAMMAR_NO_MATCH)
      return postfix;

    int call = grammarParseCallExpr(req, start, end);
    if (call != GRAMMAR_NO_MATCH)
      return call;

    return parseAtom(req, &tokens->data[start]);
  }

  // pecah kiri dan kanan
  int left = parseBinary(req, start, minIndex);
  int right = parseBinary(req, minIndex + 1, end);

  if (tokens->data[minIndex].type == ARROW)
    return createThen(req->node, left, right);
  if (tokens->data[minIndex].type == PIPE)
    return createFallback(req->node, left, right);
  return createBinary(req->node, &tokens->data[minIndex], left, right);
}
