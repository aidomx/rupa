#include <rupa/package.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fungsi utilitas
void printIndent(int level) {
  for (int i = 0; i < level; i++)
    printf("  "); // Gunakan 2 spasi untuk indentasi
}

void printBoolean(bool value, int level) {
  printIndent(level);
  printf("Boolean: %s\n", value == 1 ? "true" : "false");
}

void printFloat(char *value, int level) {
  printIndent(level);
  char *format = "Float: %s\n";

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

void printString(AstNode *n, int level) {
  printIndent(level);

  if (n->type == NODE_STRING) {
    printf("String: %s\n", n->string.value);
  }

  else if (n->type == NODE_LITERAL_ID) {
    printf("Literal ID: %s\n", n->string.value);
  }
}

// Fungsi utama untuk mencetak node AST
static void printAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_ASSIGN:
    printIndent(level);
    printf("Assignment:\n");
    printIndent(level + 1);
    printf("Target:\n");
    if (n->assign.target >= 0 && n->assign.target < node->length) {
      AstNode *target = &node->ast[n->assign.target];
      if (target->type == NODE_IDENTIFIER) {
        printId(target->identifier.name, level + 2);
      }
    }
    printIndent(level + 1);
    printf("Value:\n");
    printAst(node, n->assign.value, level + 2);
    break;

  case NODE_BOOLEAN:
    printBoolean(n->boolean.value, level);
    break;

  case NODE_FLOAT:
    printFloat(n->asFloat.lexeme, level);
    break;

  case NODE_NUMBER:
    printNumber(n->number.value, level);
    break;

  case NODE_NULLABLE:
    printNullable(n->string.value, level);
    break;

  case NODE_STRING:
    printString(n, level);
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
    printString(n, level);
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

  // Loop melalui semua node, namun hanya cetak node yang tidak memiliki orang
  // tua. Ini biasanya merupakan node akar (root node) dari setiap ekspresi atau
  // assignment.
  for (int i = 0; i < node->length; i++) {
    bool isRoot = true;
    for (int j = 0; j < node->length; j++) {
      AstNode *n = &node->ast[j];
      if ((n->type == NODE_ASSIGN &&
           (n->assign.target == i || n->assign.value == i)) ||
          (n->type == NODE_BINARY &&
           (n->binary.left == i || n->binary.right == i))) {
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
