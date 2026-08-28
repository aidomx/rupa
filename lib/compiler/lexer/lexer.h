#pragma once

/*#include "lexeme/lexeme.h"*/
#include "processor/processor.h"

#if defined(RUPA_PACKAGE_H)

extern void createSymbol(void);
extern const size_t SYMBOL_LIST_SIZE;
extern Array getArrayIndex(const char *input, int start, int end);
extern Position getSymbolIndex(const char *input, int start, int end);
extern char *getTokenId(const char *input, int start, int end);
extern char *getTokenValue(const char *input, int start, int end);
extern int handleQuotes(char c, int *in_quotes, int *quote_char);
extern int handleBraces(char c, int brace_level, State *state);
extern int handleWhitespace(const char *next_char, int in_quotes, State *state);
extern int handleRegularChar(char c, State *state);
extern int shouldAddNewline(int i, int length, char **history, State *state);
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
