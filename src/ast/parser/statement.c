#include <rupa/package.h>

/**
 * parseStatement: memproses 1 statement.
 * grammar: identifier = expression
 */
Response parseStatement(Request *req, Response res) {
  if (req->left == -1)
    return parseExpression(req, res);
  res.leftId = parseFactor(req, res);
  return parseExpression(req, res);
}
