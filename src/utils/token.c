#include <rupa/package.h>
#include <stdbool.h>
#include <stdio.h>

bool match(DataToken *data, TokenType T) { return data && data->type == T; }

bool isToken(Token *t, int pos, TokenType type) {
  return (pos >= 0 && pos < t->length && match(&t->data[pos], type));
}

DataToken *getToken(Token *t, int pos) {
  return (pos < 0 || pos >= t->length) ? NULL : &t->data[pos];
}

int lastIndex(Token *token, int start) {
  if (!token)
    return start;
  int currentLine = token->data[start].line;
  while (start < token->length && token->data[start].line == currentLine) {
    start++;
  }
  return start;
}

int findParen(Token *tokens, int start, int end) {
  if (!tokens || start >= end)
    return -1;

  int depth = 1;
  for (int i = start + 1; i < end; i++) {
    if (match(&tokens->data[i], LPAREN)) {
      depth++;
    } else if (match(&tokens->data[i], RPAREN)) {
      depth--;
      if (depth == 0)
        return i;
    }
  }
  return -1;
}
