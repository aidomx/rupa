#include <rupa.h>

static void printAst(Node *node, int index, int level);

// Fungsi utilitas
void printIndent(int level) {
  for (int i = 0; i < level; i++)
    printf("  "); // Gunakan 2 spasi untuk indentasi
}

void printBoolean(bool value, int level) {
  printIndent(level);
  printf("Boolean: %s\n", value == 1 ? "true" : "false");
}

void printDecimal(char *value, int level) {
  printIndent(level);
  char *format = "Decimal: %s\n";

  printf(format, value);
}

void printId(char *id, int level) {
  printIndent(level);
  printf("Identifier: %s\n", id ? id : "null");
}

void printNumber(int value, int level) {
  printIndent(level);
  printf("Number: %d\n", value);
}

void printNullable(char *value, int level) {
  printIndent(level);
  printf("Nullable: %s\n", value);
}

void printString(char *value, char *label, int level) {
  printIndent(level);
  printf("%s: %s\n", label, value);
}

// Fungsi utama untuk mencetak node AST
static void printAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_PROGRAM:
    printIndent(level);
    printf("Program:\n");
    AstDeclaration *current = n->program.declarations;
    while (current != NULL) {
      printAst(node, current->nodeId, level + 1);
      current = current->next;
    }
    break;

  case NODE_RETURN:
    printIndent(level);
    printf("Return:\n");
    printAst(node, n->asReturn.expression, level + 1);
    break;

  case NODE_BREAK:
    printIndent(level);
    printf("Break\n");
    break;

  case NODE_CONTINUE:
    printIndent(level);
    printf("Continue\n");
    break;

  case NODE_ARRAY:
    printIndent(level);
    if (n->array.length == 0) {
      printf("ArrayLiteral: (empty)\n");
    }

    else {
      printf("ArrayLiteral:\n");
      for (int i = 0; i < n->array.length; i++) {
        printAst(node, n->array.elements[i], level + 1);
      }
    }
    break;

  case NODE_ASSIGN:
    printIndent(level);
    printf("Assignment:\n");
    printIndent(level + 1);
    printf("Target:\n");
    if (n->assign.target >= 0 && n->assign.target < node->length) {
      AstNode *target = &node->ast[n->assign.target];
      if (target->type == NODE_IDENTIFIER) {
        printId(target->identifier.name, level + 2);
      } else if (target->type == NODE_SUBSCRIPT) {
        printAst(node, n->assign.target, level + 2);
      }
    }

    if (n->assign.type >= 0 && n->assign.type < node->length) {
      printIndent(level + 1);
      printf("Type:\n");
      printAst(node, n->assign.type, level + 2);
    }

    printIndent(level + 1);
    printf("Value:\n");
    printAst(node, n->assign.value, level + 2);
    break;

  case NODE_BOOLEAN:
    printBoolean(n->boolean.value, level);
    break;

  case NODE_DECIMAL:
    printDecimal(n->decimal.lexeme, level);
    break;

  case NODE_NUMBER:
    printNumber(n->number.value, level);
    break;

  case NODE_NULLABLE:
    printNullable(n->string.value, level);
    break;

  case NODE_STRING:
    printString(n->string.value, "String", level);
    break;

  case NODE_MEMBER:
    printIndent(level);
    printf("Member:\n");
    printIndent(level + 1);
    printf("Object:\n");
    printAst(node, n->member.object, level + 2);
    printIndent(level + 1);
    printf("Member:\n");
    printAst(node, n->member.member, level + 2);
    break;

  case NODE_SUBSCRIPT:
    printIndent(level);
    printf("Subscript:\n");

    // cetak identifier dasar
    printIndent(level + 1);
    printf("Base:\n");
    printAst(node, n->subscript.posId, level + 2);

    // cetak index ekspresi (jika ada)
    printIndent(level + 1);
    printf("Index:\n");
    if (n->subscript.index >= 0 && n->subscript.index < node->length) {
      printAst(node, n->subscript.index, level + 2);
    } else {
      printIndent(level + 2);
      printf("(empty)\n");
    }
    break;

  case NODE_IDENTIFIER:
    printId(n->identifier.name, level);
    break;

  case NODE_BINARY:
    printIndent(level);
    printf("Binary: %s\n", n->binary.op);
    printIndent(level + 1);
    printf("Left:\n");
    printAst(node, n->binary.left, level + 2);
    printIndent(level + 1);
    printf("Right:\n");
    printAst(node, n->binary.right, level + 2);
    break;

  case NODE_LITERAL_ID:
    printString(n->string.value, "Literal ID", level);
    break;

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
    break;

  case NODE_PRINT:
    printIndent(level);
    printf("Print:\n");
    for (int i = 0; i < n->print.length; i++)
      printAst(node, n->print.args[i], level + 1);
    break;

  case NODE_BLOCK:
    printIndent(level);
    printf("Block:\n");
    for (int i = 0; i < n->block.length; i++)
      printAst(node, n->block.statements[i], level + 1);
    break;

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
    break;

  case NODE_CONDITIONAL_ASSIGN:
    printIndent(level);
    printf("Conditional Assignment:\n");
    printIndent(level + 1); printf("Target:\n");
    printAst(node, n->conditionalAssign.target, level + 2);
    printIndent(level + 1); printf("Value:\n");
    printAst(node, n->conditionalAssign.value, level + 2);
    break;

  case NODE_THEN:
    printIndent(level); printf("Then:\n");
    printIndent(level + 1); printf("Condition:\n");
    printAst(node, n->then.condition, level + 2);
    printIndent(level + 1); printf("Result:\n");
    printAst(node, n->then.result, level + 2);
    break;

  case NODE_FALLBACK:
    printIndent(level); printf("Fallback:\n");
    printIndent(level + 1); printf("Primary:\n");
    printAst(node, n->fallback.primary, level + 2);
    printIndent(level + 1); printf("Fallback:\n");
    printAst(node, n->fallback.fallback, level + 2);
    break;

  case NODE_UPDATE:
    printIndent(level);
    printf("Update: %s%s\n", n->update.prefix ? "prefix " : "postfix ",
           n->update.op ? n->update.op : "?");
    if (n->update.target >= 0)
      printAst(node, n->update.target, level + 1);
    break;

  case NODE_LOOP:
    printIndent(level);
    printf("Loop: %s\n", n->loop.kind);
    if (n->loop.condition >= 0)
      printAst(node, n->loop.condition, level + 1);
    if (n->loop.body >= 0)
      printAst(node, n->loop.body, level + 1);
    break;

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
    break;

  case NODE_STRUCT_DECL:
    printIndent(level);
    printf("Struct:\n");
    printAst(node, n->asStruct.name, level + 1);
    if (n->asStruct.body >= 0)
      printAst(node, n->asStruct.body, level + 1);
    break;

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
    break;

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

  case NODE_IMPORT:
  case NODE_EXPORT:
  case NODE_EXTENDS:
    printIndent(level);
    printf("Module statement:\n");
    if (n->module.value >= 0)
      printAst(node, n->module.value, level + 1);
    break;

  case NODE_ASYNC:
    printIndent(level);
    printf("Async:\n");
    printIndent(level + 1);
    printf("Request:\n");
    printAst(node, n->async.request, level + 2);
    if (n->async.handler >= 0) {
      printIndent(level + 1);
      printf("Handler:\n");
      printAst(node, n->async.handler, level + 2);
    }
    if (n->async.timeout >= 0) {
      printIndent(level + 1);
      printf("Timeout:\n");
      printAst(node, n->async.timeout, level + 2);
    }
    break;

  case NODE_AWAIT:
    printIndent(level);
    printf("Await:\n");
    printAst(node, n->await.expression, level + 1);
    break;

  default:
    printIndent(level);
    printf("(unknown node type %d)\n", n->type);
    break;
  }
}

// Fungsi pemanggil awal
void startDebug(Node *node) {
  if (!node || node->length == 0)
    return;

  printf("--- Struktur AST Node ---\n");

  // ✅ Cari program node (biasanya node pertama)
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      printAst(node, i, 0);
      printf("--- ENDOF ---\n");
      return;
    }
  }

  // Fallback: jika tidak ada program node, print semua root nodes
  for (int i = 0; i < node->length; i++) {
    bool isRoot = true;
    for (int j = 0; j < node->length; j++) {
      AstNode *n = &node->ast[j];
      if ((n->type == NODE_ASSIGN &&
           (n->assign.target == i || n->assign.value == i)) ||
          (n->type == NODE_BINARY &&
           (n->binary.left == i || n->binary.right == i)) ||
          (n->type == NODE_SUBSCRIPT &&
           (n->subscript.posId == i || n->subscript.index == i)) ||
          (n->type == NODE_RETURN && n->asReturn.expression == i)) {
        isRoot = false;
        break;
      }
    }
    if (isRoot) {
      printAst(node, i, 0);
    }
  }

  printf("--- ENDOF ---\n");
}
