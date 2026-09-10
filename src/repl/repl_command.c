#include <rupa.h>

/**
 * @brief Handle REPL dot-commands (.help, .clear, .exit).
 *
 * These are REPL-only commands that don't go through the interpreter.
 * Returns true if a command was handled, false if input is not a command.
 */
bool handleReplCommand(State *state, ReplContext *ctx) {
  if (!state && !ctx) return false;

  Buffer *buffer = state->buffer;
  if (!buffer || buffer->length == 0) return false;

  /* Empty or blank input — ignore */
  if (isblank(*buffer->value)) return true;

  /* Not a dot-command */
  if (buffer->value[0] != '.') return false;

  if (strcmp(buffer->value, ".help") == 0) {
    help(true);
    return true;
  }

  if (strcmp(buffer->value, ".clear") == 0) {
    clearScreen();
    welcomeMessage();
    state->repl->editor->lineNumber = 0;
    return true;
  }

  if (strcmp(buffer->value, ".exit") == 0) {
    state->isRepl = false;
    clearInput(state->input);
    clearStateToken(state->tokens);
    return true;
  }

  /* Unknown dot-command — silently ignore */
  return true;
}
