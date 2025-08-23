#include <ctype.h>
#include <rupa/package.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === CLEANUP ===
void clearNode(Node *node) {
  if (!node)
    return;

  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_IDENTIFIER && node->ast[i].identifier.name) {
      free(node->ast[i].identifier.name);
    }

    if (node->ast[i].type == NODE_BINARY && node->ast[i].binary.op) {
      free(node->ast[i].binary.op);
    }

    if (node->ast[i].type == NODE_FLOAT && node->ast[i].asFloat.lexeme) {
      free(node->ast[i].asFloat.lexeme);
    }

    if (node->ast[i].type == NODE_STRING && node->ast[i].string.value) {
      free(node->ast[i].string.value);
    }

    if (node->ast[i].type == NODE_NULLABLE && node->ast[i].string.value) {
      free(node->ast[i].string.value);
    }
  }

  node->capacity = NODE_PROGRAM;
  node->length = 0;
  free(node->ast);
  free(node);
}

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

/* ====== Node Builders ====== */
int createBoolean(Node *root, bool value) {
  AstNode node = {.type = NODE_BOOLEAN, .boolean.value = value};
  return createAst(root, node);
}

int createFloat(Node *root, char *value) {
  AstNode node = {.type = NODE_FLOAT,
                  .asFloat.value = atof(value),
                  .asFloat.lexeme = strdup(value)};
  return createAst(root, node);
}

/**
 * Membuat node identifier (variabel).
 */
int createId(Node *root, char *name) {
  if (name == NULL)
    return -1;

  // karakter pertama harus huruf atau underscore
  if (!(isalpha(name[0]) || name[0] == '_')) {
    return -1;
  }

  // sisa karakter boleh huruf/angka/underscore
  for (int i = 1; name[i]; i++) {
    if (!(isalnum(name[i]) || name[i] == '_')) {
      return -1;
    }
  }

  // lolos validasi → buat identifier node
  AstNode node = {.type = NODE_IDENTIFIER, .identifier.name = strdup(name)};
  return createAst(root, node);
}

/**
 * Membuat node number literal.
 */
int createNumber(Node *root, int value) {
  AstNode node = {.type = NODE_NUMBER, .number.value = value};
  return createAst(root, node);
}

int createString(Node *root, char *value, TokenType type) {
  AstNode node = {
      .type = NODE_STRING, .string.type = type, .string.value = strdup(value)};

  if (type == NULLABLE) {
    node.type = NODE_NULLABLE;
  }

  return createAst(root, node);
}

/**
 * Membuat node binary expression.
 */
int createBinary(Node *root, DataToken *opToken, int leftId, int rightId) {
  if (!root || leftId < 0 || rightId < 0 || !opToken)
    return -1;

  char *op = strdup(opToken->value);
  BinaryType binType = getBinaryType(opToken);

  AstNode node = {
      .type = NODE_BINARY,
      .binary.left = leftId,
      .binary.right = rightId,
      .binary.op = op,
      .binary.type = binType,
  };

  return createAst(root, node);
}

/**
 * Membuat node assignment.
 */
int createAssignment(Node *root, int left, int right) {
  if (!root || left < 0 || right < 0)
    return -1;

  AstNode node = {
      .type = NODE_ASSIGN, .assign.target = left, .assign.value = right};

  return createAst(root, node);
}
