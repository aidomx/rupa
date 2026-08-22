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

  if (repl->history) {
    for (int i = 0; i < repl->history->size; i++) {
      if (repl->history->entries[i]) {
        gcfree(repl->history->entries[i]);
        repl->history->entries[i] = NULL;
      }
    }
    repl->history->size = 0;
    repl->history->currentIndex = -1;
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

  /* Every field must be reset, not just line/multiline/row. This function
   * is what test()/prompt.c calls between independent syntax test files
   * (see clearGlobalState's reuse pattern); leaving brace/bracket/paren/
   * colon/objectDepth/inStruct etc. stale here means a file that fails
   * midway through an unbalanced construct (e.g. a lexer error) leaves
   * context that corrupts parsing of the *next* file in the batch, even
   * though that next file is syntactically valid on its own. */
  ctx->flagStatus = FLAG_NONE;
  ctx->brace = 0;
  ctx->bracket = 0;
  ctx->colon = 0;
  ctx->paren = 0;
  ctx->inStrictType = 0;
  ctx->inFunc = 0;
  ctx->inStruct = 0;
  ctx->objectDepth = 0;
  ctx->line = 0;
  ctx->lineStart = 0;
  ctx->space = 0;
  ctx->expectAssignment = 0;
  ctx->row = 0;
  ctx->multiline = false;
  ctx->currentId = NULL;
  ctx->strictType = NULL;
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
    gcfree(state->tokens->data);
    state->tokens->data = NULL;
    gcfree(state->tokens);
    state->tokens = NULL;
  }

  if (state->repl) {
    /* ReplState and its children are GC-owned allocations. */
    if (state->repl->history) {
      History *history = state->repl->history;
      for (int i = 0; i < history->capacity; i++) {
        if (history->entries && history->entries[i]) {
          gcfree(history->entries[i]);
          history->entries[i] = NULL;
        }
      }
      gcfree(history->entries);
      history->entries = NULL;
      gcfree(history);
      state->repl->history = NULL;
    }

    if (state->repl->buffer) {
      gcfree(state->repl->buffer->value);
      state->repl->buffer->value = NULL;
      gcfree(state->repl->buffer);
      state->repl->buffer = NULL;
    }

    if (state->repl->editor) {
      gcfree(state->repl->editor);
      state->repl->editor = NULL;
    }

    gcfree(state->repl);
    state->repl = NULL;
  }

  if (state->context) {
    clearStateContext(state->context);
    gcfree(state->context);
    state->context = NULL;
  }

  if (state->input) {
    clearStateInput(state->input);
    gcfree(state->input);
    state->input = NULL;
  }

  state->isRepl = false;
  gcfree(state);
}
