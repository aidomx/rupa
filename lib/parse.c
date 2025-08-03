#include "ctypes.h"
#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration
static int parseExpression(Node *node, Parser *parser);

// === TOKEN HELPERS ===

DataToken *peek(Parser *parser) {
  if (!parser || !parser->tokens)
    return NULL;
  if (parser->length >= parser->tokens->length)
    return NULL;
  return &parser->tokens->data[parser->length];
}

DataToken *advance(Parser *parser) {
  if (!parser || !parser->tokens)
    return NULL;
  if (parser->length < parser->tokens->length)
    return &parser->tokens->data[parser->length++];
  return NULL;
}

bool hasTokens(Parser *parser) {
  return parser && parser->tokens && (parser->length < parser->tokens->length);
}

// === AST NODE CREATION ===

int saveNode(Node *node, NodeType nodeType, AstNode n) {
  switch (nodeType) {
  case NODE_ASSIGN:
  case NODE_IDENTIFIER:
  case NODE_NUMBER:
    return createAst(node, n);
  default:
    return 0;
  }
}

int createNumber(Node *node, DataToken *token) {
  AstNode n = {.type = NODE_NUMBER, .number.value = atoi(token->value)};
  return saveNode(node, NODE_NUMBER, n);
}

int createIdentifier(Node *node, DataToken *token) {
  AstNode n = {.type = NODE_IDENTIFIER,
               .identifier.name = strdup(token->value)};
  int index = saveNode(node, NODE_IDENTIFIER, n);

  if (!node->ast[index].identifier.name) {
    free(&node->ast[index]);
    return 0;
  }

  return index;
}

int createAssignment(Node *node, int left, int right) {
  AstNode n = {
      .type = NODE_ASSIGN, .assign.target = left, .assign.value = right};
  return saveNode(node, NODE_ASSIGN, n);
}

// === PARSER ===

// atom → IDENTIFIER | NUMBER
int parseAtom(Node *node, Parser *parser) {
  if (!parser)
    return 0;

  DataToken *token = peek(parser);
  if (!token)
    return 0;

  switch (token->type) {
  case IDENTIFIER:
    return createIdentifier(node, advance(parser));
  case NUMBER:
    return createNumber(node, advance(parser));
  default:
    return 0;
  }
}

// expression → IDENTIFIER "=" expression
//            | atom
int parseExpression(Node *node, Parser *parser) {
  int left = parseAtom(node, parser);
  DataToken *token = peek(parser);

  if (token && token->type == ASSIGN) {
    advance(parser);
    int right = parseExpression(node, parser);
    return createAssignment(node, left, right);
  }

  return left;
}

// program → expression*
Node *parseAllNodes(Parser *parser) {
  Node *node = createNode(NODE_PROGRAM);
  while (hasTokens(parser)) {
    parseExpression(node, parser);
  }
  return node;
}

// === AST PRINTER ===

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
      printf(")\n");
      break;
    }
    default:
      printf("(unknown)\n");
    }
  }
}

// === CLEANUP ===

void destroyNode(Node *node) {
  if (!node)
    return;

  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_IDENTIFIER && node->ast[i].identifier.name) {
      free(node->ast[i].identifier.name);
    }
  }

  free(node->ast);
  node->capacity = NODE_PROGRAM;
  node->length = 0;
  free(node);
}

// === ENTRY POINT ===

void parse(Token *tokens) {
  if (!tokens || tokens->length == 0)
    return;

  Parser parser = {.length = 0, .tokens = tokens};
  Node *node = parseAllNodes(&parser);

  if (node->length > 0) {
    printf("Total: %d\n", node->length);
    printAst(node);
    destroyNode(node);
  }
}
