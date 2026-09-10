#include <rupa.h>

bool printBasicAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return false;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_ARRAY_TYPE:
    printIndent(level);
    printf("ArrayType:\n");
    printAst(node, n->arrayType.elementType, level + 1);
    return true;
  case NODE_ARRAY:
    printIndent(level);
    if (n->array.length == 0) {
      printf("ArrayLiteral: (empty)\n");
    } else {
      printf("ArrayLiteral:\n");
      for (int i = 0; i < n->array.length; i++)
        printAst(node, n->array.elements[i], level + 1);
    }
    return true;
  case NODE_BOOLEAN:
    printBoolean(n->boolean.value, level);
    return true;
  case NODE_DECIMAL:
    printDecimal(n->decimal.lexeme, level);
    return true;
  case NODE_NUMBER:
    printNumber(n->number.value, level);
    return true;
  case NODE_NULLABLE:
    printNullable(n->string.value, level);
    return true;
  case NODE_STRING:
    printString(n->string.value, "String", level);
    return true;
  case NODE_IDENTIFIER:
    printId(n->identifier.name, level);
    return true;
  case NODE_LITERAL_ID:
    printString(n->string.value, "Literal ID", level);
    return true;
  default:
    return false;
  }
}
