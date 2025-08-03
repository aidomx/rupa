#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "enum.h"
#include "limit.h"
#include <stdbool.h>

struct AstNode;

typedef struct {
  int target;
  int value;
} AstAssignment;

typedef struct {
  BinaryType type;
  struct AstNode *left;
  struct AstNode *right;
} AstBinary;

typedef struct {
  bool value;
} AstBoolean;

typedef struct {
  double value;
} AstDouble;

typedef struct {
  float value;
} AstFloat;

typedef struct {
  char *name;
} AstIdentifier;

typedef struct {
  int value;
} AstNumber;

typedef struct {
  struct AstNode *declarations;
  struct AstNode *next;
} AstProgram;

typedef struct {
  struct AstNode *expression;
} AstReturn;

typedef struct {
  char *name;
} AstString;

typedef struct {
  VariableType type;
  struct AstNode *name;
  struct AstNode *next;
} AstVariable;

typedef struct {
  char *name;
  struct AstNode *value;
} Assignment;

typedef struct {
  char *value;
  int line;
  int row;
  TokenType type;
} DataToken;

typedef struct {
  char message[MAX_MESSAGE_LENGTH];
  char code[MAX_CODE_LENGTH];
  int line;
  int row;
  ErrorType type;
} ErrorInfo;

typedef struct {
  ErrorInfo info[256];
  int length;
} Error;

typedef struct {
  char name[64];
  char value[1024];
  int line;
  int row;
  int length;
} SymbolTable;

typedef struct {
  char value[1024];
  int line;
  int row;
  TokenType types;
} Args;

typedef struct {
  char *name;
} Identifier;

typedef struct {
  char func[MAX_VALUE];
  char args[8][256];
  int line;
  int row;
  TokenType types;
} Function;

typedef struct Number {
  int value;
} Number;

typedef struct Binary {
  char *op;
  struct AstNode *left;
  struct AstNode *right;
} Binary;

typedef struct AstNode {
  NodeType type;
  union {
    AstAssignment assign;
    AstBinary binary;
    AstBoolean boolean;
    AstDouble asDouble;
    AstFloat asFloat;
    AstIdentifier identifier;
    AstNumber number;
    AstProgram program;
    AstReturn asReturn;
    AstString string;
    AstVariable variable;
    DataToken *token;
  };
} AstNode;

typedef struct {
  AstNode *ast;
  int capacity;
  int length;
} Node;

typedef struct {
  char entry[MAX_BUFFER_SIZE];
  char format[64];
  char output[MAX_BUFFER_SIZE];
  int length;
} SystemConfig;

typedef struct {
  DataToken *data;
  int capacity;
  int length;
} Token;

typedef struct Parser {
  Token *tokens;
  int length;
} Parser;

typedef struct {
  char **history;
  int length;
  int line;
} ReplState;

#endif
