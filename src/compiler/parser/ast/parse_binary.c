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

/* Chain postfix operations (.member, (), []) after an inner expression. */
static int chainPostfix(Request *req, int current, int i, int end) {
  Token *tokens = req->tokens;
  while (i < end) {
    while (i < end && grammarIsWhitespace(tokens, i))
      i++;
    if (i >= end)
      break;
    if (isToken(tokens, i, DOT)) {
      i++;
      while (i < end && grammarIsWhitespace(tokens, i))
        i++;
      if (i >= end || (tokens->data[i].type != IDENTIFIER &&
                       tokens->data[i].type != LITERAL_ID))
        break;
      int member = createId(req->node, tokens->data[i].value);
      current = createMember(req->node, current, member);
      i++;
    } else if (isToken(tokens, i, LPAREN)) {
      int close = grammarMatchClose(tokens, i, end, LPAREN, RPAREN);
      if (close < 0)
        break;
      int *args = NULL, n = grammarParseArgs(req, i + 1, close, &args);
      current = createCall(req->node, current, args, n);
      i = close + 1;
    } else if (isToken(tokens, i, LBLOCK)) {
      int close = grammarMatchClose(tokens, i, end, LBLOCK, RBLOCK);
      if (close < 0)
        break;
      int idx = close == i + 1 ? -1 : grammarParseExpr(req, i + 1, close);
      current = createSubscript(req->node, current, idx);
      i = close + 1;
    } else {
      break;
    }
  }
  return current;
}

/* Ternary standar (parsir bersarang kanan): cond ? then : else.
 * Split pertama '?' pada level terluar; else = sisi kanan penuh
 * (memungkinkan else berupa ternary lagi). then: butuh ':' pasangan
 * dalam rentang then (pertimbangkan kedalaman '?'/':' supaya then
 * yang mengandung ternary sendiri tetap benar). */
static int parseTernary(Request *req, int start, int end) {
  Token *tokens = req->tokens;
  int q = -1;
  int depthParen = 0;
  for (int i = start; i < end; i++) {
    if (isToken(tokens, i, LPAREN)) depthParen++;
    else if (isToken(tokens, i, RPAREN)) depthParen--;
    else if (depthParen == 0 && isToken(tokens, i, QUESTION_MARK)) {
      q = i;
      break;
    }
  }
  if (q < 0) return GRAMMAR_NO_MATCH;

  /* then = [q+1, colon); cari ':' pada kedalaman '?' seimbang. */
  int colon = -1;
  int qDepth = 0;
  depthParen = 0;
  for (int i = q + 1; i < end; i++) {
    if (isToken(tokens, i, LPAREN)) depthParen++;
    else if (isToken(tokens, i, RPAREN)) depthParen--;
    else if (depthParen == 0 && isToken(tokens, i, QUESTION_MARK)) qDepth++;
    else if (depthParen == 0 && isToken(tokens, i, COLON)) {
      if (qDepth == 0) { colon = i; break; }
      qDepth--;
    }
  }
  if (colon < 0) return GRAMMAR_NO_MATCH;
  if (colon == q + 1 || colon + 1 >= end) return GRAMMAR_NO_MATCH;

  int cond = parseBinary(req, start, q);
  if (cond < 0) return GRAMMAR_NO_MATCH;
  int thenId = parseBinary(req, q + 1, colon);
  if (thenId < 0) return GRAMMAR_NO_MATCH;
  int elseId = parseBinary(req, colon + 1, end);
  if (elseId < 0) return GRAMMAR_NO_MATCH;

  /* Render ke AST ternary pipa yang sudah ada: cond -> then | else
   * (THEN di atas rantai FALLBACK satu segmen) — evaluator & IR
   * ternary tidak perlu diubah. */
  int fb = createFallback(req->node, thenId, elseId);
  return createThen(req->node, cond, fb);
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

  while (start < end &&
         (isToken(tokens, start, NEWLINE) || isToken(tokens, start, TAB))) {
    start++;
  }

  while (end > start &&
         (isToken(tokens, end - 1, NEWLINE) || isToken(tokens, end - 1, TAB))) {
    end--;
  }

  if (start >= end)
    return -1;

  if (tokens->data[start].type == KEYWORD) {
    int id = grammarParseAsyncExpr(req, start, end);
    if (id != GRAMMAR_NO_MATCH)
      return id;
    id = grammarParseAwaitExpr(req, start, end);
    if (id != GRAMMAR_NO_MATCH)
      return id;
  }

  /* Ternary standar `cond ? then : else` — cek sebelum split binary:
   * '?' dan ':' tak punya precedence, jadi dicocokkan eksplisit.
   * Hanya '?' level terluar (di luar bracket/paren) yang dihitung —
   * '?' dalam sub-ekspresi tidak boleh memicu parse ternary di sini. */
  {
    int depthParen = 0, depthBracket = 0;
    for (int i = start; i < end; i++) {
      if (isToken(tokens, i, LPAREN)) depthParen++;
      else if (isToken(tokens, i, RPAREN)) depthParen--;
      else if (isToken(tokens, i, LBLOCK)) depthBracket++;
      else if (isToken(tokens, i, RBLOCK)) depthBracket--;
      else if (depthParen == 0 && depthBracket == 0 &&
               isToken(tokens, i, QUESTION_MARK)) {
        int ternary = parseTernary(req, start, end);
        if (ternary != GRAMMAR_NO_MATCH) return ternary;
        break;
      }
    }
  }

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
      if (tokens->data[i].type == MINUS &&
          (i == start || !isOperand(tokens->data[i - 1].type)))
        continue;
      minPrec = prec;
      minIndex = i;
    }
  }

  if (minIndex == -1) {
    /* Unary NOT: `!expr` — operand adalah sisa ekspresi di kanannya */
    if (isToken(tokens, start, EXCLAMATION)) {
      int operand = parseBinary(req, start + 1, end);
      if (operand >= 0)
        return createNot(req->node, operand);
      return -1;
    }

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
        return parseBinary(req, start + 1, k);
      }
      /* (expr).field or (expr)() — parse inner, then chain postfixes */
      if (k > start && k + 1 < end) {
        int current = parseBinary(req, start + 1, k);
        if (current >= 0)
          return chainPostfix(req, current, k + 1, end);
      }
    }

    else if (isToken(tokens, start, LBLOCK)) {
      int k = findArr(tokens, start);
      if (k == end - 1) {
        /* Array literal, bukan grouping: parseBinary(start+1, k) akan
         * memakan koma sebagai binary dan merusak elemen ([3, 4] → 3). */
        int arr = grammarParseArrayLiteral(req, start, end);
        if (arr != GRAMMAR_NO_MATCH)
          return arr;
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

  int left = parseBinary(req, start, minIndex);
  int right = parseBinary(req, minIndex + 1, end);

  if (tokens->data[minIndex].type == ARROW)
    return createThen(req->node, left, right);
  if (tokens->data[minIndex].type == PIPE)
    return createFallback(req->node, left, right);
  return createBinary(req->node, &tokens->data[minIndex], left, right);
}
