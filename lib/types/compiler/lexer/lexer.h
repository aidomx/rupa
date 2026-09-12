#pragma once

#if defined(RUPA_PACKAGE_H)

struct Array {
  int left;
  int right;
};

struct DataToken {
  char *value;
  char *safetyType;
  int line;
  int row;
  enum TokenType type;
};

/**
 * @struct LexerState
 * @brief Menyimpan konteks sementara lexer saat memproses satu
 * program/statement.
 *
 * Struktur ini bersifat ephemeral (per eksekusi resolveProgram),
 * digunakan untuk menghindari passing parameter berulang.
 */
struct LexerState {
  struct Token *token; // Target token list
  char *at;            // Annotation type
  const char *content; // Source input content
  int cursor;          // Cursor akhir terminate
  int end;             // Cursor akhir lexeme
  int line;            // Line number
  int row;             // Column number
  int start;           // Cursor awal lexeme
};

struct Token {
  struct DataToken *data;
  int capacity;
  int length;
};

#endif
