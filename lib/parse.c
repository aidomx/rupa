#include "ctypes.h"
#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * parseAtom: memproses atom (IDENTIFIER atau NUMBER).
 */
int parseAtom(Request *req, DataToken *data) {
  if (!data)
    return -1;

  if (match(data, BOOLEAN))
    return createBoolean(req->node,
                         strcmp(data->value, "true") == 0 ? true : false);
  if (match(data, FLOAT))
    return createFloat(req->node, data->value);
  if (match(data, IDENTIFIER))
    return createId(req->node, data->value);
  if (match(data, NUMBER))
    return createNumber(req->node, atoi(data->value));
  if (match(data, STRING))
    return createString(req->node, data->value);

  return -1;
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

  // cari operator top-level (depth == 0)
  for (int i = start; i < end; i++) {
    if (match(&tokens->data[i], LPAREN)) {
      depth++;
      continue;
    }
    if (match(&tokens->data[i], RPAREN)) {
      depth--;
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
    if (match(&tokens->data[start], LPAREN)) {
      int k = findParen(tokens, start, end);
      if (k == end - 1) {
        // kupas kurung luar
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

/**
 * parseExpression: memproses ekspresi kanan assignment.
 */
Response parseExpression(Request *req, Response res) {
  if (req->right.start >= req->right.end)
    return res;

  int start = req->right.start;
  int end = req->right.end;

  res.rightId = parseBinary(req, start, end);
  return res;
}

/**
 * parseFactor: memproses faktor kiri assignment (IDENTIFIER).
 */
int parseFactor(Request *req, Response res) {
  if (req->left == -1 || req->right.start >= req->right.end)
    return -1;
  Token *tokens = req->tokens;
  DataToken *t = &tokens->data[req->left];
  return match(t, IDENTIFIER) ? createId(req->node, t->value) : -1;
}

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
