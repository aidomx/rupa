#include <rupa/package.h>

/**
 * parseExpression: memproses ekspresi kanan assignment.
 */
Response parseExpression(Request *req, Response res) {
  if (req->right.start >= req->right.end)
    return res;

  Token *t = req->tokens;
  int start = req->right.start;
  int end = req->right.end;

  // Handle expressions in parentheses
  if (match(&t->data[start], LPAREN)) {
    int rparen_pos = findParen(t, start, end);
    if (rparen_pos != -1) {
      // Parse the expression inside parentheses
      res.rightId = parseBinary(req, start + 1, rparen_pos);
      return res;
    }
  }

  else if (match(&t->data[start], LBLOCK)) {
    res.rightId = parseArray(req, res, start + 1, end);
    return res;
  }

  res.rightId = parseBinary(req, start, end);
  return res;
}
