#include <rupa.h>

static void rollback(Token *tokens, int length) {
  if (!tokens || length < 0 || length > tokens->length)
    return;
  for (int i = length; i < tokens->length; i++) {
    free(tokens->data[i].value);
    free(tokens->data[i].safetyType);
    tokens->data[i].value = NULL;
    tokens->data[i].safetyType = NULL;
  }
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
      return false;
    }
    rollback(tokens, baseline);
    input->cursor = start;
    flags->isWaiting = false;
    flags->isComplete = false;
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
