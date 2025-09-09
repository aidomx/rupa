#include <rupa/package.h>

int setDeepToken(State *s, char inner[], int pos, int length) {
  if (!s)
    return -1;

  char c = s->input[pos];

  if (issymvalue(c)) {
    if (length > 0) {
      inner[length] = '\0';
      addToken(s->tokens,
               createDataToken(inner, NULL, gettype(inner), s->line, pos));
      length = 0;
    }

    addDelim(s->tokens, c, NULL, s->line, pos);
    return length;
  }

  return -1;
}

int setDeepSubscript(State *s, int start, int end) {
  if (!s)
    return -1;

  char *input = s->input;
  char inner[1024];
  int length = 0;

  for (int i = start; i < end; i++) {
    if (setDeepToken(s, inner, i, length) > 0)
      continue;

    inner[length++] = input[i];
  }

  inner[length] = '\0';
  TokenType type = gettype(inner);
  type = type == IDENTIFIER ? LITERAL_ID : type;

  return addToken(s->tokens,
                  createDataToken(inner, NULL, type, s->line, length));
}

int setSubscript(State *state, int start, int end) {
  if (!state || start >= end)
    return -1;

  return setDeepSubscript(state, start, end);
}

int setDeepTokenId(State *s, int start, int end) {
  if (!s || start >= end)
    return -1;

  int capacity = 10;
  Array *subscripts = malloc(capacity * sizeof(Array));

  char *input = s->input;
  int array_length = 0, i = start;

  while (i < end) {
    if (isassign(input[i])) {
      end = i++;
      break;
    }

    if (isopenarray(input[i])) {
      Array arr = getArrayIndex(input, i, end);

      if (arr.left > 0 && arr.right > 0) {
        if (array_length >= capacity) {
          int newCapacity = capacity * 2;

          Array *newSubscripts =
              realloc(subscripts, newCapacity * sizeof(Array));
          if (!newSubscripts) {
            free(newSubscripts);
            return -1;
          }

          subscripts = newSubscripts;
        }

        subscripts[array_length++] = arr;
        i = arr.right;
      }
    }
    i++;
  }

  if (array_length == 0) {
    int tokenId = createTokenId(s, start, end);
    if (tokenId == -1)
      return -1;

    addDelim(s->tokens, input[end], NULL, s->line, end);
    free(subscripts);
    return end;
  }

  int tokenId = createTokenId(s, start, subscripts[0].left);
  if (tokenId == -1) {
    free(subscripts);
    return -1;
  }

  for (int index = 0; index < array_length; index++) {
    Array arr = subscripts[index];

    char prev = input[arr.left], next = input[arr.right];

    addDelim(s->tokens, prev, NULL, s->line, arr.left);
    setSubscript(s, arr.left + 1, arr.right);
    addDelim(s->tokens, next, NULL, s->line, arr.right);
  }

  addDelim(s->tokens, input[end], NULL, s->line, end);

  free(subscripts);
  return end;
}

int handleTokenId(State *state, int start, int end) {
  if (!state)
    return -1;

  char *input = state->input;
  char ptr = input[start];

  if (!(isstr(ptr) || isunderscore(ptr)))
    return -1;

  return setDeepTokenId(state, start, end);
}

int handleTokenValue(State *state, int start, int end) {
  if (!state)
    return -1;

  Token *tokens = state->tokens;
  int line = state->line;
  char *value = getTokenValue(state->input, start, end);

  if (!value)
    return start;

  char *ptr = value;
  char input[1024];
  int length = 0;

  for (int i = 0; ptr[i]; i++) {
    if (setDeepToken(state, input, i, length) > 0)
      continue;

    input[length++] = ptr[i];
  }

  input[length] = '\0';
  TokenType type = gettype(input);
  type = type == IDENTIFIER ? LITERAL_ID : type;

  int result =
      addToken(tokens, createDataToken(input, NULL, type, line, length));

  free(value);
  return result;
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
