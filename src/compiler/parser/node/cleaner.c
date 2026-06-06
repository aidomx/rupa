#include <rupa.h>

// === CLEANUP ===
void clearNode(Node *node) {
  if (!node)
    return;

  for (int i = 0; i < node->length; i++) {
    switch (node->ast[i].type) {
    case NODE_ARRAY:
      free(node->ast[i].array.elements);
      node->ast[i].array.length = 0;
      break;
    case NODE_IDENTIFIER:
      free(node->ast[i].identifier.name);
      free(node->ast[i].identifier.safetyType);
      break;
    case NODE_BINARY:
      free(node->ast[i].binary.op);
      break;
    case NODE_FLOAT:
      free(node->ast[i].asFloat.lexeme);
      break;
    case NODE_STRING:
    case NODE_LITERAL_ID:
    case NODE_NULLABLE:
      free(node->ast[i].string.value);
      break;
    case NODE_PROGRAM:
      // Free linked list declarations
      while (node->ast[i].program.declarations != NULL) {

        AstDeclaration *next = node->ast[i].program.declarations->next;
        free(node->ast[i].program.declarations);
        node->ast[i].program.declarations = next;
      }
      break;
    default:
      break;
    }
  }

  free(node->ast);
  free(node);
}
