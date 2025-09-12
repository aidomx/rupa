#pragma once

#include "rupa/package.h"

/**
 * @brief REPL state management.
 *
 * Menyimpan history input, length, current line, dan token list.
 */
struct ReplState {
  char **history;
  int length;
  int line;
  Token *tokens;
};

/**
 * @brief Global state untuk tokenization.
 *
 * Menyimpan REPL management, input buffer, tokens, dan current position.
 */
struct State {
  ReplState *manage;
  char input[MAX_BUFFER_SIZE];
  struct Token *tokens;
  int line;
  int row;
};
