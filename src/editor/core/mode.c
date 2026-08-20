#include <rupa.h>

void handleCommandMode(ReplState *repl, int key) {
  if (!repl || !repl->buffer || !repl->editor)
    return;

  switch (key) {}
}

void handleNormalMode(ReplState *repl, int key) {
  Editor *editor = repl->editor;

  switch (key) {
  case 'i':
    setEditorMode(editor, EDITOR_MODE_INSERT);
    break;
  case ':':
    setEditorMode(editor, EDITOR_MODE_COMMAND);
    break;
  case 'A':
  case ARROW_UP:
    setEditorMode(editor, EDITOR_MODE_INSERT);
    setEditorAttr(editor, EDITOR_ATTR_ARROW_UP);
    navigateHistory(repl, -1);
    break;
  case 'B':
  case ARROW_DOWN:
    setEditorMode(editor, EDITOR_MODE_INSERT);
    setEditorAttr(editor, EDITOR_ATTR_ARROW_DOWN);
    navigateHistory(repl, 1);
    break;
  case 'C':
  case ARROW_RIGHT:
    setEditorMode(editor, EDITOR_MODE_INSERT);
    setEditorAttr(editor, EDITOR_ATTR_ARROW_RIGHT);
    moveCursor(repl, 1);
    break;
  case 'D':
  case ARROW_LEFT:
    setEditorMode(editor, EDITOR_MODE_INSERT);
    setEditorAttr(editor, EDITOR_ATTR_ARROW_LEFT);
    moveCursor(repl, -1);
    break;
  case 'H':
    setEditorMode(editor, EDITOR_MODE_INSERT);
    moveCursorToStart(repl);
    break;
  case 'F':
    setEditorMode(editor, EDITOR_MODE_INSERT);
    moveCursorToEnd(repl);
    break;
  }
}

void handleInsertMode(ReplState *repl, int key) {
  Editor *editor = repl->editor;

  if (key == 27) { // ESC key
    setEditorMode(editor, EDITOR_MODE_NORMAL);
    setEditorAttr(editor, EDITOR_ATTR_ESCAPE);
    return;
  }

  switch (key) {
  // case 'C':
  case ARROW_RIGHT:
    setEditorAttr(editor, EDITOR_ATTR_ARROW_RIGHT);
    moveCursor(repl, 1);
    break;

  // case 'D':
  case ARROW_LEFT:
    setEditorAttr(editor, EDITOR_ATTR_ARROW_LEFT);
    moveCursor(repl, -1);
    break;
  case 127: // Backspace
  case '\b':
    setEditorAttr(editor, EDITOR_ATTR_BACKSPACE);
    deleteChar(repl);
    break;
  default:
    if (key >= 32 && key <= 126) {
      insertChar(repl, (char)key);
    }
  }
}
