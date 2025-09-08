#include <rupa/package.h>

int setSubscript(State *state, int start, int end) {
  if (!state || start >= end)
    return -1;

  Token *tokens = state->tokens;
  char *input = state->input;
  char inner[1024];
  int length = 0;

  for (int i = start; i < end; i++) {
    char c = input[i];

    if (issymvalue(c)) {
      if (length > 0) {
        inner[length] = '\0';
        DataToken newData = {.line = state->line,
                             .row = i,
                             .safetyType = NULL,
                             .type = gettype(inner),
                             .value = strdup(inner)};

        addToken(tokens, newData);

        length = 0;
      }
      addDelim(tokens, c, NULL, state->line, i);
      continue;
    }

    inner[length++] = c;
  }

  inner[length] = '\0';
  DataToken newData = {.line = state->line,
                       .row = length,
                       .safetyType = NULL,
                       .type = gettype(inner),
                       .value = strdup(inner)};

  return addToken(tokens, newData);
}

int handleTokenId(State *state, int start, int end) {
  if (!state)
    return -1;

  Token *tokens = state->tokens;
  char *input = state->input;
  char ptr = input[start];

  Array array = {.left = 0, .right = 0};

  if (!(isstr(ptr) || isunderscore(ptr)))
    return -1;

  for (int i = start; i < end; i++) {
    ptr = input[i];

    if (isopenarray(ptr)) {
      array = getArrayIndex(input, i, end);
    }

    if (isassign(ptr)) {
      end = i;
    }
  }

  if (array.left == 0) {
    int tokenId = createTokenId(state, start, end);
    if (tokenId == -1)
      return -1;

    addDelim(tokens, input[end], NULL, state->line, end);

    return end;
  }

  if (createTokenId(state, start, array.left) == -1)
    return -1;

  addDelim(tokens, input[array.left], NULL, state->line, array.left);
  setSubscript(state, array.left + 1, array.right);
  addDelim(tokens, input[array.right], NULL, state->line, array.right);
  addDelim(tokens, input[end], NULL, state->line, state->row);

  return end;
}

int handleTokenValue(State *state, int start, int end) {
  if (!state)
    return -1;

  Token *tokens = state->tokens;
  int line = state->line;
  // int row = state->row;
  char *value = getTokenValue(state->input, start, end);

  if (!value)
    return start;

  char *ptr = value;
  char input[1024];
  int length = 0;

  for (int i = 0; ptr[i]; i++) {
    if (issymvalue(ptr[i])) {
      if (length > 0) {
        input[length] = '\0';
        DataToken newData = {.line = line,
                             .row = i,
                             .safetyType = NULL,
                             .type = gettype(input),
                             .value = strdup(input)};

        addToken(tokens, newData);

        length = 0;
      }
      addDelim(tokens, ptr[i], NULL, line, i);
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
  return addToken(tokens, newData);
}

int handleNextToken(State *state, int start, int end) {
  if (!state)
    return -1;

  start = handleTokenId(state, start, end);

  if (start == -1)
    return -1;

  return handleTokenValue(state, start + 1, end);
}

int processToken(State *state) {
  if (!state)
    return -1;

  int hasAssign = 0, start = 0, end = 0, row = 0;

  while (row < state->row) {
    char c = state->input[row];

    if (isnewline(c)) {
      row++;
      start = row;
      continue;
    }

    if (isassign(c)) {
      if (!hasAssign) {
        hasAssign = 1;
      }
    }

    row++;
    end = row;
  }

  // Jika token adalah atom
  if (!hasAssign) {
    char *input = substring(state->input, start, end);
    DataToken data = {.line = state->line,
                      .row = state->row,
                      .safetyType = NULL,
                      .type = gettype(input),
                      .value = strdup(input)};

    free(input);
    return addToken(state->tokens, data);
  }

  return handleNextToken(state, start, end);
}
