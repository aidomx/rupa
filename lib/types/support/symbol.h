#pragma once

#if defined(RUPA_PACKAGE_H)
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
struct Symbol {
  char token;
  TokenType type;
};

extern struct Symbol symbolList[];

#endif
