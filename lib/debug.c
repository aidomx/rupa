#include "package.h"
#include "structure.h"
#include <stdio.h>

void printAst(Node *node) {
  if (!node || node->length == 0) {
    printf("(empty)\n");
    return;
  }

  for (int i = 0; i < node->length; i++) {
    AstNode *n = &node->ast[i];
    switch (n->type) {
    case NODE_IDENTIFIER:
      if (n->identifier.name)
        printf("(id %s)\n", n->identifier.name);
      else
        printf("(id null)\n");
      break;
    case NODE_NUMBER:
      printf("(num %d)\n", n->number.value);
      break;
    case NODE_ASSIGN: {
      printf("(assign ");
      AstNode *t = &node->ast[n->assign.target];
      AstNode *v = &node->ast[n->assign.value];
      if (t->type == NODE_IDENTIFIER) {
        printf("%s = ", t->identifier.name ? t->identifier.name : "null");
      }
      if (v->type == NODE_NUMBER) {
        printf("%d", v->number.value);
      }

      if (v->type == NODE_BINARY) {
        AstNode *l = &node->ast[v->binary.left];
        AstNode *r = &node->ast[v->binary.right];
        if (l->type == NODE_NUMBER)
          printf("%d ", l->number.value);
        else if (l->type == NODE_IDENTIFIER && l->identifier.name)
          printf("%s ", l->identifier.name);

        if (v->binary.op) {
          printf("%s ", v->binary.op); // Operator
        }

        if (r->type == NODE_NUMBER)
          printf("%d", r->number.value);
        else if (r->type == NODE_IDENTIFIER && r->identifier.name)
          printf("%s", r->identifier.name);
      }
      printf(")\n");
      break;
    }

    case NODE_BINARY: {
      AstNode *l = &node->ast[n->binary.left];
      AstNode *r = &node->ast[n->binary.right];
      printf("(binary ");
      if (l->type == NODE_NUMBER)
        printf("%d ", l->number.value);
      else if (l->type == NODE_IDENTIFIER && l->identifier.name)
        printf("%s ", l->identifier.name);

      if (n->binary.op)
        printf("%s ", n->binary.op); // Operator

      if (r->type == NODE_NUMBER)
        printf("%d", r->number.value);
      else if (r->type == NODE_IDENTIFIER && r->identifier.name)
        printf("%s", r->identifier.name);

      printf(")\n");
      break;
    }
    default:
      printf("(unknown)\n");
    }
  }
}
