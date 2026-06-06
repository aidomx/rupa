#include <rupa.h>

/**
 * @brief Menyimpan token dari lexer state ke token list.
 */
int saveToken(LexerState *lex) {
  if (check_lexer(lex))
    return -1;

  char *at = lex->at;
  const char *content = lex->content;
  int start = lex->start;
  int end = lex->end;
  int line = lex->line;
  int row = lex->row;

  /*end = removeChar(content, '\n', end);*/
  char *input = substring(content, start, end);
  if (!input)
    return -1;

  printf("input: %s\n", input);

  Token *token = lex->token;
  TokenType tokenType = setTokenType(input);

  DataToken data = createDataToken(input, at, tokenType, line, row);

  return addToken(token, data);
}
