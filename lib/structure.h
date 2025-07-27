#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "enum.h"
#include "limit.h"
#include <stdbool.h>

typedef struct {
  char *value;
  int line;
  int row;
  Types type;
} DataToken;

typedef struct {
  DataToken *data;
} Assignment;

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
  Types types;
} Args;

typedef struct {
  DataToken *data;
} Identifier;

typedef struct {
  char func[MAX_VALUE];
  char args[8][256];
  int line;
  int row;
  Types types;
} Function;

typedef struct {
  NodeType type;
  union {
    Assignment assign;
    Identifier identifier;
  };
} AstNode;

typedef struct Node {
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

typedef struct {
  char **history;
  int length;
  int line;
} ReplState;

#endif
