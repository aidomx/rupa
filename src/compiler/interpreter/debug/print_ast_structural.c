#include <rupa.h>

bool printStructuralAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return false;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_PROGRAM: {
    printIndent(level);
    printf("Program:\n");
    AstDeclaration *current = n->program.declarations;
    while (current) {
      printAst(node, current->nodeId, level + 1);
      current = current->next;
    }
    return true;
  }
  case NODE_ASSIGN:
    printIndent(level);
    printf("Assignment:\n");
    printIndent(level + 1);
    printf("Target:\n");
    if (n->assign.target >= 0 && n->assign.target < node->length)
      printAst(node, n->assign.target, level + 2);
    if (n->assign.type >= 0 && n->assign.type < node->length) {
      printIndent(level + 1);
      printf("Type:\n");
      printAst(node, n->assign.type, level + 2);
    }
    printIndent(level + 1);
    printf("Value:\n");
    printAst(node, n->assign.value, level + 2);
    return true;
  case NODE_MEMBER:
    printIndent(level);
    printf("Member:\n");
    printIndent(level + 1);
    printf("Object:\n");
    printAst(node, n->member.object, level + 2);
    printIndent(level + 1);
    printf("Member:\n");
    printAst(node, n->member.member, level + 2);
    return true;
  case NODE_SUBSCRIPT:
    printIndent(level);
    printf("Subscript:\n");
    printIndent(level + 1);
    printf("Base:\n");
    printAst(node, n->subscript.posId, level + 2);
    printIndent(level + 1);
    printf("Index:\n");
    if (n->subscript.index >= 0 && n->subscript.index < node->length)
      printAst(node, n->subscript.index, level + 2);
    else {
      printIndent(level + 2);
      printf("(empty)\n");
    }
    return true;
  case NODE_BINARY:
    printIndent(level);
    printf("Binary: %s\n", n->binary.op);
    printIndent(level + 1);
    printf("Left:\n");
    printAst(node, n->binary.left, level + 2);
    printIndent(level + 1);
    printf("Right:\n");
    printAst(node, n->binary.right, level + 2);
    return true;
  case NODE_CALL:
    printIndent(level);
    printf("Call:\n");
    printIndent(level + 1);
    printf("Callee:\n");
    printAst(node, n->call.callee, level + 2);
    for (int i = 0; i < n->call.length; i++) {
      printIndent(level + 1);
      printf("Arg %d:\n", i + 1);
      printAst(node, n->call.args[i], level + 2);
    }
    return true;
  case NODE_PRINT:
    printIndent(level);
    printf("Print:\n");
    for (int i = 0; i < n->print.length; i++)
      printAst(node, n->print.args[i], level + 1);
    return true;
  case NODE_BLOCK:
    printIndent(level);
    printf("Block:\n");
    for (int i = 0; i < n->block.length; i++)
      printAst(node, n->block.statements[i], level + 1);
    return true;
  case NODE_IF:
    printIndent(level);
    printf("If:\n");
    if (n->asIf.condition >= 0) {
      printIndent(level + 1);
      printf("Condition:\n");
      printAst(node, n->asIf.condition, level + 2);
    }
    if (n->asIf.thenBlock >= 0) {
      printIndent(level + 1);
      printf("Body:\n");
      printAst(node, n->asIf.thenBlock, level + 2);
    }
    if (n->asIf.elseBlock >= 0) {
      printIndent(level + 1);
      printf("Else:\n");
      printAst(node, n->asIf.elseBlock, level + 2);
    }
    return true;
  case NODE_ANNOTATION:
    printIndent(level);
    printf("Annotation:\n");
    printIndent(level + 1);
    printf("Name:\n");
    printAst(node, n->annotation.name, level + 2);
    if (n->annotation.type >= 0) {
      printIndent(level + 1);
      printf("Type:\n");
      printAst(node, n->annotation.type, level + 2);
    }
    if (n->annotation.value >= 0) {
      printIndent(level + 1);
      printf("Value:\n");
      printAst(node, n->annotation.value, level + 2);
    }
    return true;
  case NODE_FUNCTION_DECL:
    printIndent(level);
    printf("Function:\n");
    printIndent(level + 1);
    printf("Name:\n");
    printAst(node, n->function.name, level + 2);
    printIndent(level + 1);
    printf("Parameters:\n");
    for (int i = 0; i < n->function.paramLength; i++)
      printAst(node, n->function.params[i], level + 2);
    if (n->function.body >= 0) {
      printIndent(level + 1);
      printf("Body:\n");
      printAst(node, n->function.body, level + 2);
    }
    return true;
  case NODE_STRUCT_DECL:
    printIndent(level);
    printf("Struct:\n");
    printAst(node, n->asStruct.name, level + 1);
    if (n->asStruct.body >= 0)
      printAst(node, n->asStruct.body, level + 1);
    return true;
  case NODE_MEMBER_ASSIGN:
    printIndent(level);
    printf("MemberAssign:\n");
    printIndent(level + 1);
    printf("Target:\n");
    printAst(node, n->memberAssign.target, level + 2);
    printIndent(level + 1);
    printf("Value:\n");
    printAst(node, n->memberAssign.value, level + 2);
    return true;
  default:
    return false;
  }
}
