#include <rupa/package.h>

int setDeepSubscript(State *s, int start, int end) {
  if (!s)
    return -1;

  char *input = s->input;
  char inner[1024];
  int length = 0;

  for (int i = start; i < end; i++) {
    char c = input[i];

    if (issymvalue(c)) {
      if (length > 0) {
        inner[length] = '\0';
        addToken(s->tokens,
                 createDataToken(inner, NULL, gettype(inner), s->line, i));
        length = 0;
      }

      addDelim(s->tokens, c, NULL, s->line, i);
      continue;
    }
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
  Position *subscripts = malloc(capacity * sizeof(Position));

  char *input = s->input;
  int size = 0, i = start;

  while (i < end) {
    if (isassign(input[i])) {
      if (isrbracket(input[i - 1])) {
        end = i++;
        break;
      }

      end = i++;
      break;
    }

    if (islbracket(input[i]) || islblock(input[i])) {
      Position pos = getSymbolIndex(input, i, end);

      if (pos.start > 0 && pos.end > 0) {
        if (size >= capacity) {
          int newCapacity = capacity * 2;

          Position *newSubscripts =
              realloc(subscripts, newCapacity * sizeof(Position));
          if (!newSubscripts) {
            free(newSubscripts);
            return -1;
          }

          subscripts = newSubscripts;
        }

        subscripts[size++] = pos;
        i = pos.end;
      }
    }

    i++;
  }

  if (size == 0) {
    int tokenId = createTokenId(s, start, end);
    if (tokenId == -1)
      return -1;

    addDelim(s->tokens, input[end], NULL, s->line, end);
    free(subscripts);
    return end;
  }

  int tokenId = createTokenId(s, start, subscripts[0].start);
  if (tokenId == -1) {
    free(subscripts);
    return -1;
  }

  for (int index = 0; index < size; index++) {
    Position pos = subscripts[index];

    char prev = input[pos.start], next = input[pos.end];

    addDelim(s->tokens, prev, NULL, s->line, pos.start);
    setSubscript(s, pos.start + 1, pos.end);
    addDelim(s->tokens, next, NULL, s->line, pos.end);
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
    if (issymvalue(ptr[i])) {
      if (length > 0) {
        input[length] = '\0';
        addToken(tokens, createDataToken(input, NULL, gettype(input), line, i));
        length = 0;
      }

      addDelim(tokens, ptr[i], NULL, line, i);
      continue;
    }
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

// Call from src/tokenize/main.c
int processToken(State *state) {
  if (!state)
    return -1;

  char *input = state->input;
  int hasAssign = 0, hasBlock = 0;
  int start = 0;
  int end = state->row;

  for (int i = 0; i < end; i++) {
    if (isnewline(input[i])) {
      i++;
      start = i;
      continue;
    }

    if (isassign(input[i])) {
      if (!hasAssign) {
        hasAssign = 1;
      }
    }
  }

  // Jika token adalah atom
  if (!hasAssign && !hasBlock) {
    char token[64];
    int length = 0;
    for (int i = start; i < end; i++) {
      token[length++] = input[i];
    }

    if (length > 0) {
      token[length] = '\0';

      return addToken(
          state->tokens,
          createDataToken(token, NULL, gettype(token), state->line, start));
    }

    return -1;
  }

  return handleNextToken(state, start, end);
}
