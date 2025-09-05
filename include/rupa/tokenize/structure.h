#pragma once

#include "rupa/package.h"

struct DataToken {
  char *value;
  char *safetyType;
  int line;
  int row;
  TokenType type;
};

struct Token {
  DataToken *data;
  int capacity;
  int length;
};
