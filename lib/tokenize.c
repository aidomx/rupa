#include "ctypes.h"
#include "enum.h"
#include "package.h"
#include "structure.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *createToken(int capacity) {
  Token *token = malloc(sizeof(Token));

  if (token == NULL) {
    perror("Memory allocation failed.");
    exit(EXIT_FAILURE);
  }

  token->data = calloc(capacity, sizeof(DataToken));
  token->capacity = capacity;
  token->length = 0;

  return token;
}

int addToken(Token *t, TokenType type, const char *value, int line, int row) {
  if (!t || t->length >= MAX_TOKENS || !value)
    return 0;

  if (t->length >= t->capacity) {
    size_t newCapacity = t->capacity * 2;
    DataToken *data = realloc(t->data, sizeof(DataToken) * newCapacity);

    if (!data) {
      perror("Relocation of data is failed.");
      return 1;
    }

    memset(&data[t->length], 0,
           (newCapacity - t->capacity) * sizeof(DataToken));
    t->data = data;
    t->capacity = newCapacity;
  }

  DataToken *data = &t->data[t->length];
  data->value = strdup(value);
  data->line = line >= 1 ? line - 1 : line;
  data->row = row;
  data->type = type;

  return t->length++;
}

int addDelim(Token *t, char c, int line, int row) {
  const char del[2] = {c, '\0'};

  if (isassign(c)) {
    return addToken(t, ASSIGN, del, line, row);
  }

  switch (c) {
  case '+':
    return addToken(t, PLUS, del, line, row);
  case '-':
    return addToken(t, MINUS, del, line, row);
  case '*':
    return addToken(t, ASTERISK, del, line, row);
  case '/':
    return addToken(t, SLASH, del, line, row);
  case '%':
    return addToken(t, PERCENT, del, line, row);
  case '&':
    return addToken(t, AMPERSAND, del, line, row);
  case '|':
    return addToken(t, PIPE, del, line, row);
  case '^':
    return addToken(t, CARET, del, line, row);
  case '~':
    return addToken(t, TILDE, del, line, row);
  case '?':
    return addToken(t, QUESTION_MARK, del, line, row);
  case ':':
    return addToken(t, COLON, del, line, row);
  case '.':
    return addToken(t, DOT, del, line, row);
  case ',':
    return addToken(t, COMMA, del, line, row);
  case ';':
    return addToken(t, SEMICOLON, del, line, row);
  case '@':
    return addToken(t, AT, del, line, row);
  case '$':
    return addToken(t, DOLLAR, del, line, row);
  case '!':
    return addToken(t, EXCLAMATION, del, line, row);
  case '<':
    return addToken(t, LESS_THAN, del, line, row);
  case '>':
    return addToken(t, GREATER_THAN, del, line, row);
  case '=':
    return addToken(t, EQUAL_THAN, del, line, row);
  case '#':
    return addToken(t, HASHTAG, del, line, row);
  case '\\':
    return addToken(t, BACKSLASH, del, line, row);
  case '`':
    return addToken(t, BACKTICK, del, line, row);
  case '"':
    return addToken(t, QUOTE, del, line, row);
  case '\'':
    return addToken(t, SINGLE_QUOTE, del, line, row);
  case '[':
    return addToken(t, BLOCK_LEFT, del, line, row);
  case ']':
    return addToken(t, BLOCK_RIGHT, del, line, row);
  case '{':
    return addToken(t, BRACE_LEFT, del, line, row);
  case '}':
    return addToken(t, BRACE_RIGHT, del, line, row);
  case '(':
    return addToken(t, PAREN_LEFT, del, line, row);
  case ')':
    return addToken(t, PAREN_RIGHT, del, line, row);
  case '\n':
    return addToken(t, NEWLINE, del, line, row);
  case '\t':
    return addToken(t, TAB, del, line, row);
  case '\r':
    return addToken(t, CARRIAGE_RETURN, del, line, row);
  case '\b':
    return addToken(t, BACKSPACE, del, line, row);
  case '\f':
    return addToken(t, FORM_FEED, del, line, row);
  default:
    return addToken(t, UNKNOWN, del, line, row);
  }
}

void clearToken(Token *token, int capacity) {
  if (!token || token->length < 0)
    return;

  for (int i = 0; i < token->length; i++) {
    free(token->data[i].value);
    token->data[i].line = 0;
    token->data[i].row = 0;
    token->data[i].type = 0;
  }

  token->capacity = capacity;
  token->length = 0;
}

void addStringToken(Token *t, const char *value, int line, int row) {
  char temp[strlen(value) + 1];
  strcpy(temp, value);
  char qopen = temp[0];
  char qclose = temp[strlen(temp) - 1];
  addDelim(t, qopen, line, row);
  trimquote(temp);
  addToken(t, STRING, temp, line, row);
  addDelim(t, qclose, line, row);
}

void saveToken(Token *t, const char *token, char expr, int line, int row) {
  if (!t || !token)
    return;

  char bracket = getBracketType(token);
  char *ptr = strdup(token);

  if (!ptr)
    return;

  if (bracket == '(') {
    addDelim(t, bracket, line, row);
    trimbracket(ptr, bracket, 0);
  } else if (bracket == ')') {
    trimbracket(ptr, 0, bracket);
  }

  trimspace(ptr);
  TokenType type = gettype(ptr);

  if (type == STRING) {
    addStringToken(t, ptr, line, row);
  } else {
    if (type == IDENTIFIER) {
      addToken(t, LITERAL_ID, ptr, line, row);
    } else {
      addToken(t, type, ptr, line, row);
    }
  }

  if (expr > 0) {
    // char op[2] = {expr, '\0'};
    addDelim(t, expr, line, row);
  }

  if (bracket == ')') {
    addDelim(t, bracket, line, row);
  }

  free(ptr);
}

void tokenize(Token *token, const char *input, int line) {
  if (!token || !input)
    return;

  for (int i = 0; input[i]; i++) {
    if (input[i] == '\0') {
      addToken(token, ENDOF, input, line, i);
    }

    if (isassign(input[i])) {
      handleVariable(token, input, line, i);
      break;
    } else {
      TokenType type = gettype(input);
      if (type == IDENTIFIER) {
        addToken(token, type, input, line, i);
        break;
      }
    }
  }
}
