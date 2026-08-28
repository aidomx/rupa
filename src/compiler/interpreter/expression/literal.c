#include <rupa.h>
InterpreterResult interpretLiteral(Node *node, AstNode *ast) {
  switch (ast->type) {
  case NODE_NUMBER: return resultNormal(valueNumber(ast->number.value));
  case NODE_DECIMAL: return resultNormal(valueDecimal(ast->decimal.value));
  case NODE_BOOLEAN: return resultNormal(valueBoolean(ast->boolean.value));
  case NODE_STRING: return resultNormal(valueString(ast->string.value));
  case NODE_NULLABLE: return resultNormal(valueNull());
  default: return resultNormal(valueNull());
  }
}
