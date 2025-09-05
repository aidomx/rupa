#include <rupa/package.h>

static int handle_quotes(char c, int *in_quotes, int *quote_char);
static int handle_braces(char c, int brace_level, State *state);
static int handle_whitespace(const char *next_char, int in_quotes,
                             State *state);
static int handle_regular_char(char c, State *state);
static int should_add_newline(int i, int length, char **history, State *state);

/**
 * @brief Tokenize Interface
 *
 * File utama sebagai antarmuka process tokenizer.
 */
Token *tokenize(Token *tokens, char **history, int length, int line) {
  if (!tokens || length == 0) {
    return NULL;
  }

  State state = {
      .tokens = tokens, .line = line >= 1 ? line - 1 : line, .row = 0};

  int in_quotes = 0;
  int quote_char = '\0';
  int brace_level = 0;

  for (int i = 0; i < length; i++) {
    char *ptr = history[i];
    if (!ptr) {
      break;
    }

    while (*ptr) {
      // Handle quotes state
      handle_quotes(*ptr, &in_quotes, &quote_char);

      // Handle braces (only outside quotes)
      if (!in_quotes) {
        handle_braces(*ptr, brace_level, &state);

        if (*ptr == '{')
          brace_level++;
        if (*ptr == '}')
          brace_level--;
      }

      // Handle character processing
      if (isspace(*ptr) && !in_quotes) {
        handle_whitespace(ptr + 1, in_quotes, &state);
      } else {
        handle_regular_char(*ptr, &state);
      }

      // Buffer overflow protection
      if (state.row >= MAX_BUFFER_SIZE) {
        fprintf(stderr, "Buffer overflow detected\n");
        state.input[state.row] = '\0';
        return NULL;
      }

      ptr++;
    }

    // Add newline between history entries if needed
    if (should_add_newline(i, length, history, &state)) {
      state.input[state.row++] = '\n';
    }
  }

  // Finalize the input string
  if (state.row > 0) {
    state.input[state.row] = '\0';
    printf("%d,%d,%s\n", state.line, state.row, state.input);
    return tokens;
  }

  return NULL;
}

/**
 * @brief Handle quote detection and state management
 */
static int handle_quotes(char c, int *in_quotes, int *quote_char) {
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
static int handle_braces(char c, int brace_level, State *state) {
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
static int handle_whitespace(const char *next_char, int in_quotes,
                             State *state) {
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
static int handle_regular_char(char c, State *state) {
  state->input[state->row++] = c;
  return 1;
}

/**
 * @brief Determine if newline should be added between history entries
 */
static int should_add_newline(int i, int length, char **history, State *state) {
  return (i < length - 1 && history[i + 1] && state->row > 0 &&
          !isspace(state->input[state->row - 1]));
}
