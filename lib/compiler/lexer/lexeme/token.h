#pragma once
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

#endif
