#include <rupa.h>

bool check_state(State *state) {
  return (!state || state->size == 0 || !state->input ||
          state->input->length == 0);
}

/**
 * @brief Membuat dan menginisialisasi state baru untuk REPL.
 *
 * Fungsi ini mengalokasikan memori untuk struktur ReplState,
 * lalu menyetel nilai awal untuk buffer, editor, history.
 *
 * @param capacity integer untuk capacity token.
 * @return Pointer ke ReplState yang sudah dialokasikan.
 */
ReplState *createReplState(int capacity) {
  ReplState *repl = gcmall(sizeof(ReplState));
  repl->buffer = createBuffer(MAX_BUFFER_SIZE);
  repl->editor = createEditor();
  repl->history = createHistory(capacity);
  repl->capacity = capacity;
  repl->size = 0;
  return repl;
}

State *createGlobalState(int capacity, bool actived) {
  State *state = gcmall(sizeof(State));
  state->tokens = createToken(capacity);
  state->error = createError(capacity);
  state->repl = createReplState(capacity);
  state->context = createStateContext(capacity);
  state->input = createInput(capacity);
  state->debug = &debug;
  state->isRepl = actived;
  state->size = 0;
  return state;
}

void clearReplState(ReplState *repl) {
  if (!repl)
    return;

  for (int i = 0; i < repl->size; i++) {
    if (repl->history->entries[i]) {
      repl->history->entries[i][0] = '\0';
    }
  }

  if (repl->buffer->value) {
    repl->buffer->value[0] = '\0';
  }

  if (repl->editor) {
    repl->editor->attr = EDITOR_ATTR_NONE;
    repl->editor->cursorCol = 0;
    repl->editor->cursorLine = 0;
    repl->editor->cursorPos = 0;
    repl->editor->indentLevel = 0;
    repl->editor->lineNumber = 0;
  }
  repl->size = 0;
}

void clearStateContext(StateContext *ctx) {
  if (!ctx)
    return;

  ctx->line = 0;
  ctx->multiline = false;
  ctx->row = 0;
  // free(ctx);
}

void clearStateInput(Input *input) {
  if (!input)
    return;

  if (input->content) {
    // free(input->value);
    input->content = NULL;
  }

  if (input->context) {
    input->context = NULL;
  }

  input->capacity = 0;
  input->length = 0;

  if (!input->next)
    return;

  clearStateInput(input->next);
  input->next = NULL;
  // free(input);
}

void clearStateToken(Token *token) {
  if (!token)
    return;

  clearToken(token, token->capacity);
}

void clearGlobalState(State *state, int capacity) {
  if (!state)
    return;

  if (state->tokens) {
    clearToken(state->tokens, capacity);
    free(state->tokens->data);
    free(state->tokens);
  }

  if (state->repl) {
    // Untuk .exit, free semua memory
    for (int i = 0; i < state->repl->capacity; i++) {
      if (state->repl->history->entries[i]) {
        free(state->repl->history->entries[i]);
        state->repl->history->entries[i] = NULL;
      }
    }

    if (state->repl->history) {
      free(state->repl->history->entries);
      free(state->repl->history);
      state->repl->history = NULL;
    }

    if (state->repl->buffer) {
      free(state->repl->buffer->value);
      free(state->repl->buffer);
      state->repl->buffer = NULL;
    }

    if (state->repl->editor) {
      free(state->repl->editor);
      state->repl->editor = NULL;
    }

    free(state->repl);
    state->repl = NULL;
  }

  if (state->context) {
    clearStateContext(state->context);
  }

  if (state->input) {
    clearStateInput(state->input);
  }

  state->isRepl = false;
  free(state);
}
