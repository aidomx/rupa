#include <rupa.h>

static int findNewStart(History *h, int start, int end) {
  if (!h)
    return 0;

  int currentStart = start;
  for (int i = start; i < end; i++) {
    if (i < h->size - 1 && h->entries[i + 1])
      currentStart = i + 1;
  }

  return currentStart;
}

Input *addToInput(State *state) {
  if (!state)
    return NULL;

  ReplState *repl = state->repl;
  History *h = repl->history;
  Input *input = state->input;

  // re-start by newline
  int start = findNewStart(h, 0, state->size);
  for (int i = start; i < state->size; i++) {
    char *buffer = h->entries[i];
    size_t len = strlen(buffer);

    if (len + input->length + 2 >= MAX_BUFFER_SIZE) {
      fprintf(stderr, "Buffer overflow detection!\n");
      return NULL;
    }

    memcpy(input->content + input->length, buffer, len);
    input->length += len;

    // add newline
    if (input->length > 0 && input->content[input->length - 1] != '\n')
      input->content[input->length++] = '\n';
  }

  // endof string
  if (input->length == 0)
    return NULL;
  input->content[input->length] = '\0';

  return input;
}
