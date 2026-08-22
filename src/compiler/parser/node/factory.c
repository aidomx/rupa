#include <rupa.h>

static int *copyIds(const int *ids, int length);

int createAst(Node *node, AstNode n) {
  if (!node)
    return -1;

  if (node->length >= node->capacity) {
    int newCapacity = node->capacity * 2;

    AstNode *ast = gcresize(node->ast, sizeof(AstNode) * node->capacity,
                            sizeof(AstNode) * newCapacity);
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
  node.array.elements = copyIds(elements, length);
  node.array.length = length;
  return createAst(root, node);
}

/* ====== Node Builders ====== */
int createBoolean(Node *root, bool value) {
  AstNode node = {.type = NODE_BOOLEAN, .boolean.value = value};
  return createAst(root, node);
}

int createDecimal(Node *root, char *value) {
  AstNode node = {.type = NODE_DECIMAL,
                  .decimal.value = strtod(value, NULL),
                  .decimal.lexeme = gcstrdup(value)};
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
  AstNode node = {.type = NODE_IDENTIFIER, .identifier.name = gcstrdup(name)};
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

int createBreak(Node *root) {
  AstNode node = {.type = NODE_BREAK};
  return createAst(root, node);
}

int createContinue(Node *root) {
  AstNode node = {.type = NODE_CONTINUE};
  return createAst(root, node);
}

int createUpdate(Node *root, int target, const char *op, bool prefix) {
  if (!root || target < 0 || !op)
    return -1;

  AstNode node = {.type = NODE_UPDATE};
  node.update.target = target;
  node.update.op = gcstrdup(op);
  node.update.prefix = prefix;
  return createAst(root, node);
}

int createString(Node *root, char *value, NodeType nodeType) {
  AstNode node = {.type = nodeType,
                  .string.type = gettype(value),
                  .string.value = gcstrdup(value)};

  return createAst(root, node);
}

/**
 * Membuat node binary expression.
 */
int createBinary(Node *root, DataToken *opToken, int leftId, int rightId) {
  if (!root || leftId < 0 || rightId < 0 || !opToken)
    return -1;

  char *op = gcstrdup(opToken->value);
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
int createAssignment(Node *root, int left, int type, int right) {
  if (!root || left < 0 || right < 0)
    return -1;

  AstNode node = {
      .type = NODE_ASSIGN,
      .assign.target = left,
      .assign.type = type,
      .assign.value = right};

  return createAst(root, node);
}

static int *copyIds(const int *ids, int length) {
  if (length <= 0)
    return NULL;
  int *out = gcmall(sizeof(int) * length);
  if (!out)
    return NULL;
  memcpy(out, ids, sizeof(int) * length);
  return out;
}

int createCall(Node *root, int callee, int *args, int length) {
  AstNode n = {.type = NODE_CALL};
  n.call.callee = callee;
  n.call.args = copyIds(args, length);
  n.call.length = length;
  return createAst(root, n);
}

int createPrint(Node *root, int *args, int length) {
  AstNode n = {.type = NODE_PRINT};
  n.print.args = copyIds(args, length);
  n.print.length = length;
  return createAst(root, n);
}

int createBlock(Node *root, int *items, int length) {
  AstNode n = {.type = NODE_BLOCK};
  n.block.statements = copyIds(items, length);
  n.block.length = length;
  return createAst(root, n);
}

int createIf(Node *root, int condition, int thenBlock, int elseBlock) {
  AstNode n = {.type = NODE_IF};
  n.asIf.condition = condition;
  n.asIf.thenBlock = thenBlock;
  n.asIf.elseBlock = elseBlock;
  return createAst(root, n);
}

int createLoop(Node *root, const char *kind, int condition, int body) {
  AstNode n = {.type = NODE_LOOP};
  n.loop.kind = gcstrdup(kind ? kind : "");
  n.loop.condition = condition;
  n.loop.body = body;
  return createAst(root, n);
}

int createFunctionDecl(Node *root, int name, int *params, int paramLength,
                       int body) {
  AstNode n = {.type = NODE_FUNCTION_DECL};
  n.function.name = name;
  n.function.params = copyIds(params, paramLength);
  n.function.paramLength = paramLength;
  n.function.body = body;
  return createAst(root, n);
}

int createStructDecl(Node *root, int name, int body) {
  AstNode n = {.type = NODE_STRUCT_DECL};
  n.asStruct.name = name;
  n.asStruct.body = body;
  return createAst(root, n);
}

int createAnnotation(Node *root, int name, int type, int value) {
  AstNode n = {.type = NODE_ANNOTATION};
  n.annotation.name = name;
  n.annotation.type = type;
  n.annotation.value = value;
  return createAst(root, n);
}

int createModule(Node *root, NodeType type, int value) {
  AstNode n = {.type = type};
  n.module.value = value;
  return createAst(root, n);
}

int createObject(Node *root, struct AstObjectEntry *entries, int length) {
  AstNode n = {.type = NODE_OBJECT};
  if (length > 0) {
    n.object.entries = gcmall(sizeof(struct AstObjectEntry) * length);
    if (!n.object.entries)
      return -1;
    memcpy(n.object.entries, entries, sizeof(struct AstObjectEntry) * length);
  }
  n.object.length = length;
  return createAst(root, n);
}
