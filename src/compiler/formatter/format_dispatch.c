#include <rupa.h>

void fmtNode(Formatter *f, Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return;

  AstNode *n = &node->ast[id];
  switch (n->type) {
  case NODE_IDENTIFIER:
    fmtIdentifier(f, node, id);
    break;
  case NODE_LITERAL_ID:
    fmtLiteralId(f, node, id);
    break;
  case NODE_NUMBER:
    fmtNumber(f, node, id);
    break;
  case NODE_DECIMAL:
    fmtDecimal(f, node, id);
    break;
  case NODE_BOOLEAN:
    fmtBoolean(f, node, id);
    break;
  case NODE_STRING:
    fmtString(f, node, id);
    break;
  case NODE_NULLABLE:
    fmtNull(f);
    break;
  case NODE_BINARY:
    fmtBinary(f, node, id);
    break;
  case NODE_CALL:
    fmtCall(f, node, id);
    break;
  case NODE_PRINT:
    fmtPrint(f, node, id);
    break;
  case NODE_ARRAY:
    fmtArray(f, node, id);
    break;
  case NODE_OBJECT:
    fmtObject(f, node, id);
    break;
  case NODE_MEMBER:
    fmtMember(f, node, id);
    break;
  case NODE_SUBSCRIPT:
    fmtSubscript(f, node, id);
    break;
  case NODE_UPDATE:
    fmtUpdate(f, node, id);
    break;
  case NODE_ANNOTATION:
    fmtAnnotation(f, node, id);
    break;
  case NODE_ARRAY_TYPE:
    fmtArrayType(f, node, id);
    break;
  case NODE_RETURN:
    fmtReturn(f, node, id);
    break;
  case NODE_THEN:
    fmtThen(f, node, id);
    break;
  case NODE_FALLBACK:
    fmtFallback(f, node, id);
    break;
  case NODE_ASSIGN:
    fmtAssign(f, node, id);
    break;
  case NODE_CONDITIONAL_ASSIGN:
    fmtConditionalAssign(f, node, id);
    break;
  case NODE_BLOCK:
    fmtBlock(f, node, id);
    break;
  case NODE_IF:
    fmtIf(f, node, id);
    break;
  case NODE_LOOP:
    fmtLoop(f, node, id);
    break;
  case NODE_FUNCTION_DECL:
    fmtFunctionDecl(f, node, id);
    break;
  case NODE_STRUCT_DECL:
    fmtStructDecl(f, node, id);
    break;
  case NODE_IMPORT:
    fmtModule(f, node, id);
    break;
  case NODE_MODULE_IMPORT:
    fmtModuleImport(f, node, id);
    break;
  case NODE_EXPORT:
  case NODE_EXPORT_DECL:
    fmtExport(f, node, id);
    break;
  case NODE_ASYNC:
    fmtAsync(f, node, id);
    break;
  case NODE_AWAIT:
    fmtStr(f, "await ");
    fmtNode(f, node, n->await.expression);
    break;
  case NODE_CASE:
    fmtCase(f, node, id);
    break;
  case NODE_BREAK:
    fmtStr(f, "break");
    break;
  case NODE_CONTINUE:
    fmtStr(f, "continue");
    break;
  case NODE_MEMBER_ASSIGN:
    fmtNode(f, node, n->memberAssign.target);
    fmtStr(f, " = ");
    fmtNode(f, node, n->memberAssign.value);
    break;
  default:
    break;
  }
}
