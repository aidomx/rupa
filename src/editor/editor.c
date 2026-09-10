#include <rupa.h>

Editor *createEditor(void) {
  Editor *ed = gcmall(sizeof(Editor));
  ed->attr = EDITOR_ATTR_NONE;
  ed->cursorCol = 0;
  ed->cursorLine = 0;
  ed->cursorPos = 0;
  ed->indentLevel = 0;
  ed->lineNumber = 1;
  ed->mode = EDITOR_MODE_INSERT;
  ed->sequence[0] = '\0';
  ed->lineStackTop = -1;
  for (int i = 0; i < MAX_LINE_STACK; i++) {
    ed->lineStack[i].text = NULL;
    ed->lineStack[i].indent = 0;
    ed->lineStack[i].cursorPos = 0;
  }
  return ed;
}

/**
 * Save current line to the rollback stack (before Enter in multiline mode).
 */
void editorPushLine(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer) return;

  Editor *ed = repl->editor;
  Buffer *buf = repl->buffer;

  /* Don't save empty lines */
  if (buf->length == 0) return;

  /* Stack full — shift everything down, drop oldest */
  if (ed->lineStackTop >= MAX_LINE_STACK - 1) {
    if (ed->lineStack[0].text) gcfree(ed->lineStack[0].text);
    for (int i = 0; i < MAX_LINE_STACK - 1; i++) {
      ed->lineStack[i] = ed->lineStack[i + 1];
    }
    ed->lineStackTop = MAX_LINE_STACK - 2;
  }

  ed->lineStackTop++;
  SavedLine *sl = &ed->lineStack[ed->lineStackTop];
  sl->text = gcmall(buf->length + 1);
  memcpy(sl->text, buf->value, buf->length);
  sl->text[buf->length] = '\0';
  sl->indent = ed->indentLevel;
  sl->cursorPos = buf->length;
}

/**
 * Restore previous line from the rollback stack (on backspace at pos 0).
 * Returns true if a line was restored, false if stack is empty.
 */
bool editorPopLine(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer) return false;

  Editor *ed = repl->editor;
  if (ed->lineStackTop < 0) return false;

  SavedLine *sl = &ed->lineStack[ed->lineStackTop];
  if (!sl->text) return false;

  Buffer *buf = repl->buffer;
  int len = strlen(sl->text);

  if (len >= buf->capacity) return false;

  memcpy(buf->value, sl->text, len);
  buf->value[len] = '\0';
  buf->length = len;

  ed->indentLevel = 0;
  ed->cursorPos = sl->cursorPos;
  ed->cursorCol = sl->cursorPos;
  ed->lineNumber--;

  if (repl->state && repl->state->input) {
    repl->state->input->length = 0;
    repl->state->input->content[0] = '\0';

    Token *tokens = repl->state->tokens;
    Flags *fl = repl->state->input->flags;
    if (fl) fl->isWaiting = false;
    if (tokens->length > 0) clearStateToken(tokens);

    clearStateContext(repl->state->context);
  }

  gcfree(sl->text);
  sl->text = NULL;
  ed->lineStackTop--;

  printf("\033[2K\033[A\r");
  fflush(stdout);

  return true;
}

/**
 * Free all saved lines in the rollback stack.
 */
void editorClearLineStack(Editor *ed) {
  if (!ed) return;
  for (int i = 0; i <= ed->lineStackTop; i++) {
    if (ed->lineStack[i].text) {
      gcfree(ed->lineStack[i].text);
      ed->lineStack[i].text = NULL;
    }
  }
  ed->lineStackTop = -1;
}

// pintu masuk utama untuk editor
void handleKeyPress(ReplState *state, int key) {
  if (!state || !state->editor)
    return;

  // ./mode.c
  switch (state->editor->mode) {
  case EDITOR_MODE_NORMAL:
    handleNormalMode(state, key);
    break;
  case EDITOR_MODE_INSERT:
    handleInsertMode(state, key);
    break;
  case EDITOR_MODE_COMMAND:
    handleCommandMode(state, key);
    break;
  }
}
