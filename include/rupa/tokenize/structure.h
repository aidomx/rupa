#pragma once

#include "rupa/package.h"

struct Array {
  int left;
  int right;
};

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
