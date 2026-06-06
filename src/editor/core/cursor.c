#include <rupa.h>

void moveCursor(ReplState *repl, int direction) {
  if (!repl)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  int newPos = ed->cursorPos + direction;

  switch (ed->attr) {
  case EDITOR_ATTR_ARROW_LEFT:
    if (ed->cursorPos > 0) {
      ed->cursorPos--;
      ed->cursorCol--;
    }
    break;

  case EDITOR_ATTR_ARROW_RIGHT:
    if (ed->cursorPos < buf->length) {
      ed->cursorPos++;
      ed->cursorCol++;
    }
    break;

  default:
    if (newPos > 0 && newPos <= buf->length) {
      ed->cursorCol = newPos;
      ed->cursorPos = newPos;
    }
  }

  refreshDisplay(repl);
}

void moveCursorToEnd(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer)
    return;

  repl->editor->cursorCol = repl->buffer->length;
  repl->editor->cursorPos = repl->buffer->length;
  refreshDisplay(repl);
}

void moveCursorToStart(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer)
    return;

  repl->editor->cursorCol = 0;
  repl->editor->cursorPos = 0;
  refreshDisplay(repl);
}

void moveCursorToWordStart(ReplState *state) {
  if (!state || !state->editor || !state->buffer ||
      state->editor->cursorPos == 0)
    return;

  int pos = state->editor->cursorPos;
  char *buffer = state->buffer->value;

  // Move backwards until we find non-whitespace
  while (pos > 0 && isspace(buffer[pos - 1])) {
    pos--;
  }

  // Move backwards until we find whitespace or start
  while (pos > 0 && !isspace(buffer[pos - 1])) {
    pos--;
  }

  state->editor->cursorPos = pos;
  state->editor->cursorCol = pos;
  refreshDisplay(state);
}

void moveCursorToWordEnd(ReplState *state) {
  if (!state || !state->editor || !state->buffer)
    return;

  int pos = state->editor->cursorPos;
  int length = state->buffer->length;
  char *buffer = state->buffer->value;

  // If at end, do nothing
  if (pos >= length)
    return;

  // Move forwards until we find whitespace or end
  while (pos < length && !isspace(buffer[pos])) {
    pos++;
  }

  // Move forwards until we find non-whitespace or end
  while (pos < length && isspace(buffer[pos])) {
    pos++;
  }

  state->editor->cursorPos = pos;
  state->editor->cursorCol = pos;
  refreshDisplay(state);
}

void setCursorPosition(ReplState *state, int newPos) {
  if (!state || !state->editor || !state->buffer)
    return;

  if (newPos >= 0 && newPos <= state->buffer->length) {
    state->editor->cursorPos = newPos;
    state->editor->cursorCol = newPos;
    refreshDisplay(state);
  }
}

int getCursorScreenColumn(ReplState *state) {
  if (!state || !state->editor)
    return 0;

  // Calculate screen column considering tabs and other special characters
  int screenCol = 0;
  for (int i = 0; i < state->editor->cursorPos; i++) {
    if (state->buffer->value[i] == '\t') {
      screenCol += 4 - (screenCol % 4); // Tab to next multiple of 4
    } else {
      screenCol++;
    }
  }

  return screenCol;
}

void updateCursorLineInfo(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;
  // Count lines from start to cursor position
  int lines = 1;
  int col = 1;

  for (int i = 0; i < ed->cursorPos; i++) {
    if (buf->value[i] == '\n') {
      lines++;
      col = 1;
    } else {
      col++;
    }
  }

  ed->cursorLine = lines;
  ed->cursorCol = col;
}

void updateEditorCursor(Editor *editor, int newPos) {
  if (!editor)
    return;
  editor->cursorPos = newPos;
  // Update line and col based on buffer content if needed
}
