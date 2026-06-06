#include <rupa.h>

bool check_lexer(LexerState *lex) {
  return (!lex || !lex->content || !lex->token || lex->end < lex->start);
}

/**
 * @brief Inisialisasi LexerState dari editor, input, dan token.
 * @param editor Editor aktif
 * @param input Input aktif
 * @param token Target token list
 * @return LexerState terinisialisasi
 */
LexerState init_lex(Editor *editor, Input *input, Token *token) {
  if (!editor || !input || !token)
    return (LexerState){0};

  return (LexerState){.at = NULL,
                      .content = input->content,
                      .end = 0,
                      .line = editor->lineNumber,
                      .row = editor->cursorCol,
                      .start = 0,
                      .token = token};
}

void *lexer(State *state) {
  if (check_state(state))
    return NULL;

  return (!getKeyword(state->input)) ? process(state, "program")
                                     : process(state, "keyword");
}
