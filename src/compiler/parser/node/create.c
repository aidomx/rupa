#include <rupa.h>

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
