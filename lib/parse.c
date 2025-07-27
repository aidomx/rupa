#include "ctypes.h"
#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parseAssignment(Node *node, DataToken *data) {
  if (!data || !node)
    return;

  if (node->length >= node->capacity) {
    int newCapacity = node->capacity * 2;
    AstNode *ast = realloc(node->ast, sizeof(AstNode) * newCapacity);

    if (!ast) {
      perror("Reallocaton is failed.");
      return;
    }

    memset(&ast[node->length], 0,
           (newCapacity - node->capacity) * sizeof(AstNode));
    node->ast = ast;
    node->capacity = newCapacity;
  }

  AstNode *ast = &node->ast[node->length];
  ast->type = NODE_ASSIGN;

  if (data->type == IDENTIFIER) {
    ast->identifier.data = data;
    printf("(IDENTIFIER) %s\n", ast->assign.data->value);
  } else {
    if (data->type == EXPRESSION) {
    } else {
      ast->assign.data = data;
      printf("(VALUE) %s\n", ast->assign.data->value);
    }
  }

  node->length++;
}

//  Memparser hasil token menjadi AST
void parse(Token *token) {
  if (!token)
    return;

  Node *node = createNode(10);
  int current = 0;

  for (int i = 0; i < token->length; i++) {
    DataToken *data = &token->data[current];

    // skip =
    if (data->type == ASSIGN) {
      current++;
      continue;
    }

    parseAssignment(node, data);
    current++;
  }

  node->capacity = 10;
  node->length = 0;
  free(node->ast);
  free(node);
}
