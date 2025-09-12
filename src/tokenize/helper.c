#include <rupa/package.h>

Position getSymbolIndex(const char *input, int start, int end) {
  Position pos = {.start = 0, .end = 0};

  if (!input)
    return pos;

  int depth = 0;
  for (size_t i = 0; i < SYMBOL_TABLE_SIZE; i++) {
    if (symbol_table[i].symbol == input[start]) {
      char open = input[start];
      char close = symbol_table[i + 1].symbol;

      for (int j = start; j < end; j++) {
        if (input[j] == open)
          depth++;
        else if (input[j] == close) {
          depth--;

          if (depth == 0) {
            pos.start = start;
            pos.end = j;

            return pos;
          }
        }
      }
    }
  }

  return pos;
}

Array getArrayIndex(const char *input, int start, int end) {
  Array array = {.left = 0, .right = 0};
  if (!input)
    return array;

  int depth = 1;
  for (int i = start; i < end; i++) {
    if (islbracket(input[i]))
      depth++;
    else if (isrbracket(input[i])) {
      depth--;

      if (depth >= 0) {
        array.left = start;
        array.right = i;
        break;
      }
    }
  }

  return array;
}

char *getTokenId(const char *input, int start, int end) {
  return (!input || end == 0) ? NULL : substring(input, start, end);
}

char *getTokenValue(const char *input, int start, int end) {
  return (!input || end == 0) ? NULL : substring(input, start, end);
}

/**
 * @brief Handle quote detection and state management
 */
int handle_quotes(char c, int *in_quotes, int *quote_char) {
  if (isquote(c) && !*in_quotes) {
    *in_quotes = 1;
    *quote_char = c;
    return 1;
  } else if (*quote_char == c && *in_quotes) {
    *in_quotes = 0;
    *quote_char = '\0';
    return 1;
  }
  return 0;
}

/**
 * @brief Handle brace characters and formatting
 */
int handle_braces(char c, int brace_level, State *state) {
  if (islblock(c)) {
    if (state->row > 0 && !isspace(state->input[state->row - 1])) {
      state->input[state->row++] = ' ';
    }
    return 1;
  } else if (isrblock(c)) {
    if (brace_level == 0 && state->row > 0) {
      return 1; // Skip this character
    }
  }
  return 0;
}

/**
 * @brief Handle whitespace characters with compression
 */
int handle_whitespace(const char *next_char, int in_quotes, State *state) {
  if (in_quotes) {
    return 0;
  }

  if (state->row > 0 && !isspace(state->input[state->row - 1])) {
    state->input[state->row++] = ' ';
  }

  // Skip consecutive whitespaces
  while (isspace(*next_char)) {
    next_char++;
  }

  return 1;
}

/**
 * @brief Handle regular (non-whitespace) characters
 */
int handle_regular_char(char c, State *state) {
  state->input[state->row++] = c;
  return 1;
}

/**
 * @brief Determine if newline should be added between history entries
 */
int should_add_newline(int i, int length, char **history, State *state) {
  return (i < length - 1 && history[i + 1] && state->row > 0 &&
          !isspace(state->input[state->row - 1]));
}
