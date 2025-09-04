#include <rupa/package.h>
#include <stdio.h>
#include <stdlib.h>

void addToProgram(Node *node, int programId, int declId) {
  if (!node || programId < 0 || programId >= node->length || declId < 0 ||
      declId >= node->length) {
    return;
  }

  if (node->ast[programId].type != NODE_PROGRAM) {
    fprintf(stderr, "Error: Node %d is not a program node\n", programId);
    return;
  }

  AstDeclaration *new_decl = malloc(sizeof(AstDeclaration));
  if (!new_decl) {
    perror("Failed to allocate declaration node");
    return;
  }

  new_decl->nodeId = declId;
  new_decl->next = NULL;

  AstDeclaration **current = &node->ast[programId].program.declarations;
  while (*current != NULL) {
    current = &(*current)->next;
  }
  *current = new_decl;
}

Response processAssignment(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  int leftId = getLeftSide(t, init);

  if (leftId >= 0 && !isToken(t, leftId, IDENTIFIER)) {
    return res;
  } else if (leftId == -1) {
    fprintf(stderr, "Error: assignment at index %d has no left-hand side\n",
            init);
    return res;
  }

  int rightId = isToken(t, init + 1, ASSIGN) ? init + 2 : -1;
  if (rightId == -1) {
    fprintf(stderr, "Error: assignment at index %d has no right-hand side\n",
            init);
    return res;
  }

  req->left = leftId;
  req->right.start = rightId;
  req->right.end = lastIndex(t, init);
  res = parseStatement(req, res);

  if (res.leftId == -1 && res.rightId == -1) {
    fprintf(stderr, "Error: invalid assignment expression near index %d\n",
            init);
    return res;
  }

  int nodeId = createAssignment(req->node, res.leftId, res.rightId);

  if (nodeId == -1) {
    fprintf(stderr, "Error: failed to create assignment node at index %d\n",
            init);
    return res;
  }

  addToProgram(req->node, req->programId, nodeId);
  res.nodeId = nodeId;

  return res;
}

Response processStandaloneExpression(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  int end = lastIndex(t, init);
  int expr_id = parseBinary(req, init, end);

  if (expr_id >= 0) {
    int return_id = createReturn(req->node, expr_id);
    addToProgram(req->node, req->programId, return_id);
    res.nodeId = return_id;
  } else {
    fprintf(stderr, "Error: failed to parse expression starting at index %d\n",
            init);
  }

  return res;
}

Response generateHandler(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  if (!req || !t || init < 0 || init >= t->length)
    return res;

  if (isAssignmentStatement(t, init)) {
    return processAssignment(req, t, init);
  }

  int standalone = isStandaloneToken(t, init);

  if (standalone >= 0)
    return processStandaloneExpression(req, t, init);

  handleUnexpectedToken(req, getToken(t, init), init);
  return res;
}

void handleUnexpectedToken(Request *req, DataToken *data, int init) {
  if (req->right.end >= req->tokens->length && !match(data, ENDOF)) {
    fprintf(stderr, "Warning: unexpected token '%s' (type %d) at index %d\n",
            data->value, data->type, init);
  }
}
