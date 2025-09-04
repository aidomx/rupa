#include <rupa/package.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Membuat request parser baru dari token.
 */
Request createRequest(Token *tokens, int capacity) {
  Request req = {.tokens = tokens,
                 .node = createNode(capacity),
                 .left = -1,
                 .right.start = -1,
                 .right.end = -1,
                 .programId = createProgram(req.node)};

  return req;
}

/**
 * processGenerate: loop utama yang membangun AST dari token.
 */
Node *processGenerate(Request *req) {
  if (!req || !req->tokens)
    return NULL;

  Token *t = req->tokens;
  int pos = 0;

  // Skip whitespace tokens at the beginning
  while (pos < t->length && !isToken(t, pos, ENDOF)) {
    if (isToken(t, pos, NEWLINE) || isToken(t, pos, TAB)) {
      pos++;
      continue;
    }
    break;
  }

  // Process tokens to build AST
  int i = pos;
  while (i < t->length && !isToken(t, i, ENDOF)) {
    Response res = generateHandler(req, t, i);

    if (res.nodeId >= 0) {
      int end = lastIndex(t, i);
      if (end > i) {
        i = end;
        continue;
      }
    }

    // If we couldn't process this token, skip it or handle error
    if (isToken(t, i, NEWLINE) || isToken(t, i, TAB)) {
      i++;
      continue;
    }

    // Unknown token, skip and continue
    i++;
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
