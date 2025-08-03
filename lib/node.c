#include "enum.h"
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

int createAst(Node *node, AstNode n) {
  if (!node)
    return 0;

  if (node->length >= node->capacity) {
    int newCapacity = node->capacity * 2;

    AstNode *ast = realloc(node->ast, sizeof(AstNode) * newCapacity);
    if (!ast) {
      perror("Reallocation AstNode is failed.");
      exit(1);
    }

    memset(&ast[node->capacity], 0,
           sizeof(AstNode) * (newCapacity - node->capacity));
    node->ast = ast;
    node->capacity = newCapacity;
  }

  node->ast[node->length] = n;
  return node->length++;
}
