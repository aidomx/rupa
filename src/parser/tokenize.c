#include <ctype.h>
#include <rupa/package.h>
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

int addNewToken(Token *tokens, DataToken newData) {
  if (!tokens || tokens->length >= MAX_TOKENS)
    return 0;

  if (tokens->length >= tokens->capacity) {
    size_t newCapacity = tokens->capacity * 2;
    DataToken *data = realloc(tokens->data, sizeof(DataToken) * newCapacity);

    if (!data) {
      perror("Relocation of data is failed.");
      return 1;
    }

    memset(&data[tokens->length], 0,
           (newCapacity - tokens->capacity) * sizeof(DataToken));
    tokens->data = data;
    tokens->capacity = newCapacity;
  }

  newData.line = newData.line >= 1 ? newData.line - 1 : newData.line;
  tokens->data[tokens->length] = newData;
  return tokens->length++;
}

// @deprecated
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
  data->safetyType = NULL;

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
    return addToken(t, STAR, del, line, row);
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
    return addToken(t, LBLOCK, del, line, row);
  case ']':
    return addToken(t, RBLOCK, del, line, row);
  case '{':
    return addToken(t, LBRACE, del, line, row);
  case '}':
    return addToken(t, RBRACE, del, line, row);
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
    if (token->data[i].value) {
      free(token->data[i].value);
    }

    if (token->data[i].safetyType) {
      free(token->data[i].safetyType);
    }
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

int createTokenId(State *state, int length) {
  if (!state)
    return -1;

  Token *tokens = state->manage->tokens;
  char *id = substring(state->input, 0, length);
  int line = state->manage->line;

  DataToken data = {.line = line,
                    .row = length,
                    .safetyType = NULL,
                    .type = IDENTIFIER,
                    .value = strdup(id)};

  free(id);
  return addNewToken(tokens, data);
}

int setInput(State *state, int start, int end) {
  if (!state || start >= end)
    return -1;

  Token *tokens = state->manage->tokens;
  int line = state->manage->line;

  char *input = state->input;
  char inner[64];
  int out = 0;

  for (int i = start; i < end; i++) {
    if (!isoperator(input[i])) {
      inner[out++] = input[i];
    }
  }

  if (out > 0) {
    inner[out] = '\0';
    DataToken newData = {.line = line,
                         .row = out,
                         .safetyType = NULL,
                         .type = gettype(inner),
                         .value = strdup(inner)};

    return addNewToken(tokens, newData);
  }

  return -1;
}

typedef struct {
  int left;
  int right;
} Array;

int handleTokenId(State *state) {
  if (!state)
    return -1;

  Token *tokens = state->manage->tokens;
  char *tokenId = getTokenId(state->input, state->row);
  int line = state->manage->line;
  int row = state->row;
  char c = state->input[row];

  if (!(isstr(tokenId[0]) || isunderscore(tokenId[0])))
    return -1;

  int length = strlen(tokenId);

  Array arr = {.left = 0, .right = 0};
  char *ptr = tokenId;

  for (int i = 0; ptr[i]; i++) {
    if (ptr[i] == '[') {
      int depth = 1;

      for (int j = i; j < length; j++) {
        if (tokenId[j] == '[')
          depth++;
        else if (tokenId[j] == ']') {
          depth--;
          arr.left = i;
          arr.right = j;
        }
      }
    }
  }

  if (arr.left == 0) {
    if (createTokenId(state, length) == -1) {
      free(tokenId);
      return -1;
    }

    addDelim(tokens, c, line, row);
    free(tokenId);
    return tokens->length;
  }

  if (createTokenId(state, arr.left) == -1) {
    free(tokenId);
    return -1;
  }

  addDelim(tokens, tokenId[arr.left], line, arr.left);
  setInput(state, arr.left + 1, arr.right);
  addDelim(tokens, tokenId[arr.right], line, arr.right);
  addDelim(tokens, c, line, row);
  free(tokenId);
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

  for (int i = 0; ptr[i]; i++) {
    if (issymvalue(ptr[i])) {
      if (length > 0) {
        input[length] = '\0';
        addToken(tokens, gettype(input), input, line, i);
        length = 0;
      }
      addDelim(tokens, ptr[i], line, i);
      continue;
    }

    input[length++] = ptr[i];
  }

  input[length] = '\0';
  TokenType type = gettype(input);

  DataToken newData = {.line = line,
                       .row = length,
                       .safetyType = NULL,
                       .type = type == IDENTIFIER ? LITERAL_ID : type,
                       .value = strdup(input)};

  free(value);
  return addNewToken(tokens, newData);
}

int createVar(State *state) {
  if (!state)
    return -1;

  int tokenId = handleTokenId(state);
  if (tokenId == -1)
    return -1;

  return handleTokenValue(state);
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
    DataToken newData = {.line = line,
                         .row = row,
                         .safetyType = NULL,
                         .type = gettype(input),
                         .value = strdup(input)};

    return addNewToken(tokens, newData);
  }

  return createVar(state);
}

Token *tokenize(ReplState *state) {
  if (state && state->length == 0)
    return NULL;

  State newState = {.manage = state, .row = 0};

  for (int i = 0; i < newState.manage->length; i++) {
    if (!newState.manage->history[i])
      break;

    snprintf(newState.input, MAX_BUFFER_SIZE, "%s",
             newState.manage->history[i]);
  }

  int process = processToken(&newState);
  if (process == -1)
    return NULL;

  return newState.manage->tokens;
}
