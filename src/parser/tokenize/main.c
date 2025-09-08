#include <rupa/package.h>

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

    if (processToken(&state) == -1)
      return NULL;

    return tokens;
  }

  return NULL;
}
