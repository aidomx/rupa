#include <rupa/package.h>

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

int setSafetyType(State *state, int start, int end) {
  if (!state || start >= end)
    return -1;

  char *input = state->input;

  for (int i = start; i < end; i++) {
    if (iscolon(input[i])) {
      end = i;
    }
  }

  return end;
}

int createTokenId(State *state, int start, int end) {
  if (!state)
    return -1;

  Token *tokens = state->tokens;
  int _end = setSafetyType(state, start, end);
  char *id = getTokenId(state->input, start, _end);

  start = _end + 1;
  char *safetyType = substring(state->input, start, end);

  DataToken data = {.line = state->line,
                    .row = state->row,
                    .safetyType = NULL,
                    .type = IDENTIFIER,
                    .value = strdup(id)};

  free(id);

  if (safetyType) {
    data.safetyType = strdup(safetyType);
    free(safetyType);
  }

  return addToken(tokens, data);
}

int addToken(Token *tokens, DataToken data) {
  if (!tokens || tokens->length >= MAX_TOKENS)
    return -1;

  if (tokens->length >= tokens->capacity) {
    size_t newCapacity = tokens->capacity * 2;
    DataToken *old_data =
        realloc(tokens->data, sizeof(DataToken) * newCapacity);

    if (!old_data) {
      perror("Relocation of data is failed.");
      return -2;
    }

    memset(&old_data[tokens->length], 0,
           (newCapacity - tokens->capacity) * sizeof(DataToken));
    tokens->data = old_data;
    tokens->capacity = newCapacity;
  }

  tokens->data[tokens->length] = data;
  tokens->length++;

  return 0;
}

DataToken createDataToken(char *input, char *safetyType, TokenType tokenType,
                          int line, int row) {
  DataToken data = {.line = line,
                    .row = row,
                    .safetyType = safetyType,
                    .type = tokenType,
                    .value = strdup(input)};

  return data;
}
