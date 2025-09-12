#include <rupa/package.h>

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
      minPrec = prec;
      minIndex = i;
    }
  }

  // tidak ada operator di level atas
  if (minIndex == -1) {
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

    return parseAtom(req, &tokens->data[start]);
  }

  // pecah kiri dan kanan
  int left = parseBinary(req, start, minIndex);
  int right = parseBinary(req, minIndex + 1, end);

  return createBinary(req->node, &tokens->data[minIndex], left, right);
}
