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

/*void addChildNodes(NodeTree *node, DataToken *data) {*/
/*if (node->length >= node->capacity) {*/
/*int newCapacity = node->capacity * 2;*/
/*childNodes *children =*/
/*realloc(node->childNodes, sizeof(childNodes) * newCapacity);*/

/*if (!children) {*/
/*perror("Reallocaton is failed.");*/
/*return;*/
/*}*/

/*memset(&children[node->length], 0,*/
/*(newCapacity - node->capacity) * sizeof(childNodes));*/
/*node->childNodes = children;*/
/*node->capacity = newCapacity;*/
/*}*/

/*node->childNodes[node->length].children.value = strdup(data->value);*/
/*}*/

/*void addParentNode(NodeTree *node, DataToken *data) {*/
/*if (node->length >= node->capacity) {*/
/*int newCapacity = node->capacity * 2;*/
/*childNodes *parent =*/
/*realloc(node->childNodes, sizeof(childNodes) * newCapacity);*/

/*if (!parent) {*/
/*perror("Reallocaton is failed.");*/
/*return;*/
/*}*/

/*memset(&parent[node->length], 0,*/
/*(newCapacity - node->capacity) * sizeof(NodeTree));*/
/*node->childNodes = parent;*/
/*node->capacity = newCapacity;*/
/*}*/

/*node->childNodes[node->length].parent.value = strdup(data->value);*/
/*node->length++;*/
/*}*/

void createAssignment(NodeTree *node, DataToken *data) {

  if (!data || !node)
    return;

  if (node->type == NODE_ASSIGN) {
    if (data->type == IDENTIFIER) {
      printf("(IDENTIFIER) %s\n", data->value);
    } else {
      printf("(VALUE) %s\n", data->value);
    }
  }
}

void parse(Token *token) {
  if (!token)
    return;

  NodeTree *node = createNode(10);
  int current = 0;

  for (int i = 0; i < token->length; i++) {
    DataToken *data = &token->entries[current];

    if (data->type == ASSIGN) {
      node->type = NODE_ASSIGN;
      current++;
      continue;
    }

    createAssignment(node, data);
    current++;
  }

  node->type = 0;
  free(node);
}
