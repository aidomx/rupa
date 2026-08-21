#include <rupa.h>

/* AST memory is owned by the global GC. Clearing a Node only invalidates
 * the local view; gcclean() performs the actual release. */
void clearNode(Node *node) {
  if (!node)
    return;
  for (int i = 0; i < node->length; i++)
    memset(&node->ast[i], 0, sizeof(AstNode));
  node->length = 0;
}
