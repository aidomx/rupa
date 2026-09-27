#include <rupa.h>

static void rollback(Token *tokens, int length) {
  if (!tokens || length < 0 || length > tokens->length)
    return;
  /* Token value & safetyType dialokasi lewat GC (gcstrdup di
   * createDataToken) — JANGAN di-free() di sini: free() pada pointer
   * GC = invalid free, dan gcclean akan double-free block yang sama
   * (valgrind: "Invalid free ... by gcclean <- loader"). Rollback
   * cukup membuang entri token; memory dibiarkan untuk GC. */
  tokens->length = length;
}

static bool scan(State *state) {
  Input *input = state->input;
  Token *tokens = state->tokens;
  Flags *flags = input->flags;
  int start = input->cursor;
  int baseline = tokens->length;
  bool waiting = false;
  int result = processConstruct(state, start, input->length, &waiting);

  if (result < 0) {
    if (waiting) {
      input->cursor = start;
      flags->isWaiting = true;
      flags->isComplete = false;
      addSourceErrorAt(state->error, "LexerError", "incomplete input",
                       input->content, start, ERR_UNEXPECTED_EOF);
      return false;
    }
    rollback(tokens, baseline);
    input->cursor = start;
    flags->isWaiting = false;
    flags->isComplete = false;
    addSourceErrorAt(state->error, "LexerError", "invalid input",
                     input->content, start, ERR_UNEXPECTED_CHAR);
    return false;
  }

  input->cursor = result;
  flags->isWaiting = waiting;
  flags->isComplete = !waiting;
  return !waiting;
}

void *process(State *state) {
  if (check_state(state) || !state->input || !state->tokens)
    return NULL;
  scan(state);
  return state;
}
