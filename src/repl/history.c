#include <rupa/package.h>
#include <stdlib.h>
#include <string.h>

void clearAll(ReplState *state) {
  if (!state)
    return;

  clearToken(state->tokens, 10);
  free(state->tokens->data);
  free(state->tokens);
  clearState(state);
  free(state);
}

void clearState(ReplState *state) {
  if (!state)
    return;

  for (int i = 0; i < state->length; i++) {
    free(state->history[i]);
  }

  free(state->history);
  state->history = NULL;
  state->line = 1;
  state->length = 0;
}

void addToHistory(ReplState *state, const char *input) {
  state->history =
      realloc(state->history, (state->length + 1) * sizeof(char *));
  state->history[state->length] = strdup(input);
  state->length++;
}
