#include <rupa.h>

void printAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return;

  if (printBasicAst(node, index, level) ||
      printStructuralAst(node, index, level) ||
      printControlAst(node, index, level))
    return;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_OBJECT:
    printIndent(level);
    printf("Object:\n");
    for (int i = 0; i < n->object.length; i++) {
      printIndent(level + 1);
      printf("Entry %d:\n", i + 1);
      printIndent(level + 2);
      printf("Key:\n");
      printAst(node, n->object.entries[i].key, level + 3);
      printIndent(level + 2);
      printf("Value:\n");
      printAst(node, n->object.entries[i].value, level + 3);
    }
    break;
  default:
    printIndent(level);
    printf("(unknown node type %d)\n", n->type);
    break;
  }
}
