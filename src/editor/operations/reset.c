#include <rupa.h>

void resetEditorState(ReplState *repl) {
  if (!repl)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  buf->value[0] = '\0';
  buf->length = 0;

  ed->cursorPos = 0;
  ed->cursorCol = 0;
  ed->cursorLine = 0;
  // ed->indentLevel = 0;
  ed->lineNumber++;

  // ed->attr = EDITOR_ATTR_NONE;
  // Keep current mode
}
