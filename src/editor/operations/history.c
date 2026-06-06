#include <rupa.h>

History *createHistory(int capacity) {
  History *h = gcmall(sizeof(History));
  h->capacity = capacity;
  h->currentIndex = -1;
  h->entries = gcmall(capacity * sizeof(char *));
  if (!h->entries) {
    free(h->entries);
    return NULL;
  }

  for (int i = 0; i < capacity; i++) {
    h->entries[i] = NULL;
  }
  h->size = 0;
  return h;
}

History *addToHistory(State *state) {
  if (!state || !state->repl)
    return NULL;

  ReplState *repl = state->repl;
  Buffer *buf = repl->buffer;
  History *h = repl->history;

  if (h->size >= h->capacity) {
    if (h->entries[0] != NULL) {
      free(h->entries[0]);
      h->entries[0] = NULL;
    }

    memmove(h->entries, h->entries + 1, (h->capacity - 1) * sizeof(char *));
    h->size--;
    h->entries[h->capacity - 1] = NULL;
  }

  h->entries[h->size] = gcmall(buf->length + 1);
  if (!h->entries[h->size])
    return NULL;

  strncpy(h->entries[h->size], buf->value, buf->length);
  h->entries[h->size][buf->length] = '\0';
  h->size++;
  h->currentIndex = h->size;
  state->size++;

  return h;
}

void navigateHistory(ReplState *repl, int direction) {
  if (!repl || !repl->history)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;
  History *history = repl->history;

  int idx = history->currentIndex + direction;

  if (idx >= 0 && idx <= history->size) {
    history->currentIndex = idx;

    if (idx == history->size) {
      // Current input (not from history)
      buf->value[0] = '\0';
      buf->length = 0;
    } else {
      // Load from history
      strcpy(buf->value, history->entries[idx]);
      buf->length = strlen(buf->value);
    }

    ed->cursorPos = buf->length;
    refreshDisplay(repl);
  }
}
