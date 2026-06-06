#include <rupa.h>

int createAst(Node *node, AstNode n) {
  if (!node)
    return -1;

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

int createArray(Node *root, int *elements, int length) {
  AstNode node = {.type = NODE_ARRAY};
  node.array.elements = elements;
  node.array.length = length;

  return createAst(root, node);
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
int createId(Node *root, char *name, char *safetyType) {
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
  AstNode node = {.type = NODE_IDENTIFIER,
                  .identifier.name = strdup(name),
                  .identifier.safetyType = NULL};

  if (safetyType) {
    node.identifier.safetyType = strdup(safetyType);
  }
  return createAst(root, node);
}

/**
 * Membuat node number literal.
 */
int createNumber(Node *root, int value) {
  AstNode node = {.type = NODE_NUMBER, .number.value = value};
  return createAst(root, node);
}

/**
 * Membuat program node (root node)
 */
int createProgram(Node *root) {
  AstNode node = {.type = NODE_PROGRAM, .program.declarations = NULL};
  return createAst(root, node);
}

/**
 * Membuat return statement untuk REPL expressions
 */
int createReturn(Node *root, int expression_id) {
  AstNode node = {.type = NODE_RETURN, .asReturn.expression = expression_id};
  return createAst(root, node);
}

int createString(Node *root, char *value, NodeType nodeType) {
  AstNode node = {.type = nodeType,
                  .string.type = gettype(value),
                  .string.value = strdup(value)};

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

int createSubscript(Node *root, int posId, int index) {
  AstNode node = {.type = NODE_SUBSCRIPT,
                  .subscript.posId = posId,
                  .subscript.index = index};

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
