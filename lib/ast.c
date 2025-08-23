#include "ctypes.h"
#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ====== Utility untuk token ====== */

/**
 * match: cek apakah token sesuai dengan tipe T.
 */
bool match(DataToken *data, TokenType T) {
  return data && data->type == T ? true : false;
}

/**
 * Membuat request parser baru dari token.
 */
Request createRequest(Token *tokens, int capacity) {
  Request req = {0};
  req.tokens = tokens;
  req.node = createNode(capacity);
  req.left = -1; // belum ada target assignment
  return req;
}

/* ====== Operator Helpers ====== */

/**
 * Mengubah token operator menjadi enum BinaryType internal.
 */
BinaryType getBinaryType(DataToken *token) {
  if (!token)
    return BINARY_NONE;
  switch (token->type) {
  case ASSIGN:
    return BINARY_ASSIGN;
  case ASTERISK:
    return BINARY_MULTIPLY;
  case MINUS:
    return BINARY_SUBTRACT;
  case PLUS:
    return BINARY_ADD;
  case SLASH:
    return BINARY_DIVIDE;
  default:
    return BINARY_NONE;
  }
}

/**
 * Mendapatkan precedence operator.
 * Angka lebih tinggi = precedence lebih kuat.
 */
int getPrecedence(DataToken *token) {
  if (!token)
    return -1;
  switch (token->type) {
  case ASTERISK:
  case SLASH:
    return 3;
  case MINUS:
  case PLUS:
    return 2;
  case ASSIGN:
    return 1;
  default:
    return -1;
  }
}

/* ====== Helpers Parsing ====== */

/**
 * Mengambil index terakhir token dalam 1 baris.
 */
int lastIndex(Token *token, int start) {
  if (!token)
    return start;
  int currentLine = token->data[start].line;
  while (start < token->length && token->data[start].line == currentLine) {
    start++;
  }
  return start;
}

/**
 * findParen: mencari kurung penutup yang cocok.
 */
int findParen(Token *tokens, int start, int end) {
  if (!tokens || start >= end)
    return -1;

  int depth = 1;
  for (int i = start + 1; i < end; i++) {
    if (match(&tokens->data[i], LPAREN))
      depth++;
    else if (match(&tokens->data[i], RPAREN)) {
      depth--;
      if (depth == 0)
        return i;
    }
  }
  return -1;
}

/**
 * processGenerate: loop utama yang membangun AST dari token.
 */
Node *processGenerate(Request *req) {
  if (!req)
    return NULL;

  Response res = {0};
  Token *tokens = req->tokens;

  for (int i = 0; i < tokens->length; i++) {
    if (match(&tokens->data[i], ASSIGN)) {
      int pos = i;

      while (pos >= 0) {
        if (!match(&tokens->data[pos], IDENTIFIER)) {
          pos--;
          continue;
        }

        if (pos == 0)
          break;
        else
          pos--;
      }

      // grammar: identifier = expression
      /*if (!match(&tokens->data[pos], IDENTIFIER)) {*/
      /*break;*/
      /*}*/

      req->left = pos;
      req->right.start = i + 1;
      req->right.end = lastIndex(tokens, i);

      res = parseStatement(req, res);
      createAssignment(req->node, res.leftId, res.rightId);
    }
  }

  return req->node;
}

/**
 * generateAst: entry point parser dari token list.
 */
void generateAst(Token *tokens) {
  if (!tokens || tokens->length == 0) {
    printf("--- EMPTY ---\n");
    return;
  }

  printf("Total token: %d\n", tokens->length);

  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);

  if (node->length > 0) {
    printf("Total node: %d\n", node->length);
    startDebug(node); // debug print AST
  }

  clearNode(node);
}
