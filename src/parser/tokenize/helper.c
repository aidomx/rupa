#include <rupa/package.h>

Array getArrayIndex(const char *input, int start, int end) {
  Array array = {.left = 0, .right = 0};
  if (!input)
    return array;

  int depth = 1;
  for (int i = start; i < end; i++) {
    if (isopenarray(input[i]))
      depth++;
    else if (isclosearray(input[i])) {
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
  if (c == '{') {
    if (state->row > 0 && !isspace(state->input[state->row - 1])) {
      state->input[state->row++] = '\n';
    }
    return 1;
  } else if (c == '}') {
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
