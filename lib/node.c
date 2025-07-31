#include "structure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node *createNode(int capacity) {
  Node *node = malloc(sizeof(Node));

  if (!node) {
    perror("Memory allocation for node tree is failed.");
    exit(1);
  }

  node->ast = calloc(capacity, sizeof(AstNode));
  node->capacity = capacity;
  node->length = 0;

  return node;
}

AstNode *createAst(Node *node) {
  if (!node)
    return NULL;

  if (node->length >= node->capacity) {
    int newCapacity = node->capacity * 2;

    AstNode *ast = realloc(node->ast, sizeof(AstNode) * newCapacity);
    if (!ast) {
      perror("Reallocation AstNode is failed.");
      exit(1);
    }

    node->ast = ast;
    node->capacity = newCapacity;
  }

  AstNode *ast = &node->ast[node->length];
  memset(ast, 0, sizeof(AstNode));

  return ast;
}
