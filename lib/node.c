#include "structure.h"
#include <stdio.h>
#include <stdlib.h>

NodeTree *createNode(int capacity) {
  NodeTree *node = malloc(sizeof(NodeTree));

  if (!node) {
    perror("Memory allocation for node tree is failed.");
    exit(1);
  }

  node->type = 0;
  return node;
}
