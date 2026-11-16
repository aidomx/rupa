#pragma once

/*#include "lexeme/lexeme.h"*/
#include "processor/processor.h"

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
extern void createSymbol(void);
extern const size_t SYMBOL_LIST_SIZE;
extern char *getTokenId(const char *input, int start, int end);
extern char *getTokenValue(const char *input, int start, int end);
extern Symbol *getSymbolToken(char c);
extern Position getTokenIndex(Token *t, int start);

/**
 * @brief Validasi lexer state sebelum operasi token.
 * @param lex Pointer ke LexerState
 * @return true jika lexer state tidak valid
 */
bool check_lexer(LexerState *lex);

/**
 * @brief Inisialisasi LexerState dari editor, input, dan token.
 * @param editor Editor aktif
 * @param input Input aktif
 * @param token Target token list
 * @return LexerState terinisialisasi
 */
LexerState init_lex(Editor *editor, Input *input, Token *token);

extern void *lexer(State *state);

#endif
