#include "ctypes.h"
#include "enum.h"
#include "limit.h"
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
  if (!t || t->length >= MAX_TOKENS)
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
    return addToken(t, LPAREN, del, line, row);
  case ')':
    return addToken(t, RPAREN, del, line, row);
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
  if (!token)
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

// @deprecated
int addStringToken(Token *t, const char *value, int line, int row) {
  char temp[strlen(value) + 1];
  strcpy(temp, value);
  char qopen = temp[0];
  char qclose = temp[strlen(temp) - 1];
  addDelim(t, qopen, line, row);
  trimquote(temp);
  addToken(t, STRING, temp, line, row);
  addDelim(t, qclose, line, row);

  return t->length;
}

char *getTokenId(const char *input, int length) {
  if (!input || length == 0)
    return NULL;

  return substring(input, 0, length);
}

char *getTokenValue(const char *input, int length) {
  if (!input || length == 0)
    return NULL;

  return substring(input, length + 1, strlen(input));
}

int handleTokenId(State *state) {
  if (!state)
    return -1;

  Token *tokens = state->manage->tokens;
  int line = state->manage->line;

  char *id = getTokenId(state->input, state->row);

  if (id && !issymdenied(id[0])) {
    for (int i = 0; id[i]; i++) {
      if (issymdenied(id[i])) {
        if (!isarray(id[i])) {
          printf("\033[1;30mUndefined\033[0m\n");
        } else {
          addToken(tokens, IDENTIFIER, id, line, state->row - 1);
          addDelim(tokens, state->input[state->row], line, state->row);
        }
      } else {
        addToken(tokens, IDENTIFIER, id, line, state->row - 1);
        addDelim(tokens, state->input[state->row], line, state->row);
      }
    }
  }

  free(id);
  return tokens->length;
}

int handleTokenValue(State *state) {
  if (!state)
    return -1;

  Token *tokens = state->manage->tokens;
  int line = state->manage->line;
  int row = state->row;
  char *value = getTokenValue(state->input, row);
  char *ptr = value;
  char input[MAX_BUFFER_SIZE];
  int length = 0;

  trimspace(ptr);

  while (*ptr) {
    if (issymbol(*ptr)) {
      if (length > 0) {
        input[length] = '\0';
        addToken(tokens, gettype(input), input, line, row);
        length = 0;
      }
      addDelim(tokens, *ptr, line, row++);
      ptr++;
      row++;
      continue;
    }

    input[length++] = *ptr;
    ptr++;
    row++;
  }

  if (length > 0) {
    input[length] = '\0';
    TokenType type = gettype(input);

    switch (type) {
    case IDENTIFIER:
      addToken(tokens, LITERAL_ID, input, line, row);
      break;

    default:
      addToken(tokens, type, input, line, row);
      break;
    }
  }

  free(value);
  return tokens->length;
}

int createVar(State *state) {
  if (!state)
    return -1;

  if (handleTokenId(state)) {
    return handleTokenValue(state);
  }

  return -1;
}

int processToken(State *state) {
  if (!state)
    return -1;

  Token *tokens = state->manage->tokens;
  char *input = state->input;
  int hasAssign = 0;
  int line = state->manage->line;
  int row = state->row;

  for (; input[state->row]; state->row++) {
    if (isassign(input[state->row])) {
      hasAssign = 1;
      break;
    }
  }

  if (!hasAssign) {
    return addToken(tokens, gettype(input), input, line, row);
  }

  return createVar(state);
}

Token *tokenize(ReplState *state) {
  if (state && state->length == 0)
    return NULL;

  State newState = {.manage = state, .row = 0};

  for (int i = 0; i < state->length; i++) {
    if (!state->history[i])
      break;

    serialize(state->history[i], newState.input);
  }

  if (!processToken(&newState))
    return NULL;

  return state->tokens;
}
