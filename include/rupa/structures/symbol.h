#pragma once

#include "rupa/package.h"

/**
 * @brief Symbol table entry.
 *
 * Menyimpan nama symbol, value, dan lokasi deklarasi.
 */
struct SymbolTable {
  char name[64];
  char value[1024];
  int line;
  int row;
  int length;
};

/**
 * @brief Token untuk symbol characters.
 *
 * Menyimpan symbol character dan tipe tokennya.
 */
struct SymbolToken {
  char symbol;
  TokenType type;
};

extern SymbolToken symbol_table[];
extern const size_t SYMBOL_TABLE_SIZE;
