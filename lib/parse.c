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

// Mengatur penugasan token berdasarkan identifier
void setNodeAssign();

// Mengatur token boolean
void setNodeBoolean();

// Mengatur token float
void setNodeFloat();

// Mengatur token identifier
void setTokenId();

// Mengatur token number
void setNodeNumber();

// Mengatur expresi token
void setNodeExpression();

// Mendapatkan token terakhir
DataToken *getCurrentToken(Parser *parser, int currentIndex) {
  if (currentIndex < parser->tokens->length) {
    return &parser->tokens->data[currentIndex];
  }

  return NULL;
}

// Cek token yang masih tersimpan
bool getMoreToken(Parser *parser) {
  return parser->length < parser->tokens->length;
}

// Lanjut ke token berikutnya
int advanceToken(Parser *parser) { return parser->length++; }

// Evaluasi parse token
void evalute();

int addNode(Node *node, AstNode *ast, DataToken *token) {
  if (!ast || !node || !token)
    return 0;

  if (token->type == IDENTIFIER) {
    ast->type = NODE_IDENTIFIER;
    ast->identifier.name = strdup(token->value);
  }

  node->length++;
  return 1;
}

Node *parseNodeProgram(Parser *parser) {
  Node *node = createNode(10);

  if (!node) {
    perror("Create node program is failed.");
    exit(1);
  }

  int added_node = 0;

  while (getMoreToken(parser)) {
    DataToken *currentToken = getCurrentToken(parser, parser->length);

    if (currentToken == NULL || currentToken->type == ENDOF)
      break;

    AstNode *newAstNode = createAst(node);
    if (!newAstNode) {
      perror("Error: Failed to prepare new AstNode, out of memory.");
      return NULL;
    }

    if (addNode(node, newAstNode, currentToken) == 0) {
      fprintf(stderr,
              "Failed to add data for token '%s' at index %d, skipping...",
              currentToken->value, parser->length);
      advanceToken(parser);
      continue;
    }

    added_node++;

    if (newAstNode->type == NODE_IDENTIFIER) {
      advanceToken(parser);
      printf("%s\n", currentToken->value);
    }

    advanceToken(parser);
  }

  if (added_node == 0 && node->length == 0) {
    fprintf(stderr,
            "Error: failed is empty or parsing failed is completely.\n");
    free(node);
    return NULL;
  }

  return node;
}

//  Memparser hasil token menjadi AST
void parse(Token *token) {
  Parser parser = {.tokens = token, .length = 0};

  if (!parser.tokens) {
    return;
  }

  Node *node = parseNodeProgram(&parser);

  if (!node) {
    return;
  }

  for (int i = 0; i < node->length; i++) {
    free(node->ast[i].identifier.name);
  }

  free(node->ast);
  free(node);
}
