#pragma once

#include "lexeme/lexeme.h"
#include "processor.h"
#include "resolve/resolve.h"

#if defined(RUPA_PACKAGE_H)
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
