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
  return ed;
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
