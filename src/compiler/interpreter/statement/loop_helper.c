#include <rupa.h>

const char *identName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length)
    return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER)
    return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID)
    return ast->string.value;
  return NULL;
}

bool isRangeLoop(const AstNode *ast) {
  return ast && ast->loop.kind &&
         (!strcmp(ast->loop.kind, "for") || !strcmp(ast->loop.kind, "rev"));
}

bool rangeOperator(const char *kind, const char *op) {
  if (!kind || !op)
    return false;
  if (!strcmp(kind, "for"))
    return !strcmp(op, "<") || !strcmp(op, "<=");
  return !strcmp(op, ">") || !strcmp(op, ">=");
}
