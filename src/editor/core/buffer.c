#include <rupa.h>

Buffer *createBuffer(int capacity) {
  Buffer *buf = gcmall(sizeof(Buffer));
  buf->capacity = capacity;
  buf->length = 0;
  buf->value = gccalloc(capacity, sizeof(char));
  return buf;
}

void insertChar(ReplState *repl, char c) {
  if (!repl)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  if (buf->length >= buf->capacity - 1)
    return;

  /*if (c == '\n') {*/

  /*int currentIndent = getCurrentIndent(repl);*/

  /*memmove(&buf->value[ed->cursorPos + 1], &buf->value[ed->cursorPos],*/
  /*buf->length - ed->cursorPos + 1);*/

  /*buf->value[ed->cursorPos] = c;*/
  /*buf->length++;*/
  /*buf->value[buf->length] = '\0';*/
  /*ed->cursorPos++;*/
  /*ed->cursorLine++;*/
  /*ed->cursorCol = 1;*/
  /*ed->indentLevel = currentIndent;*/

  /*if (ed->indentLevel > 0) {*/
  /*int spaces = ed->indentLevel * 2;*/
  /*for (int i = 0; i < spaces; i++) {*/
  /*if (buf->length >= buf->capacity - 1)*/
  /*break;*/

  /*memmove(&buf->value[ed->cursorPos + 1], &buf->value[ed->cursorPos],*/
  /*buf->length - ed->cursorPos + 1);*/

  /*buf->value[ed->cursorPos] = c;*/
  /*buf->length++;*/
  /*buf->value[buf->length] = '\0';*/
  /*ed->cursorPos++;*/
  /*ed->cursorLine++;*/
  /*ed->cursorCol++;*/
  /*}*/
  /*}*/
  /*}*/

  else {
    memmove(&buf->value[ed->cursorPos + 1], &buf->value[ed->cursorPos],
            buf->length - ed->cursorPos + 1);

    buf->value[ed->cursorPos] = c;
    buf->length++;
    buf->value[buf->length] = '\0';
    ed->cursorPos++;
    ed->cursorCol++;
  }

  refreshDisplay(repl);
}

void deleteChar(ReplState *repl) {
  if (!repl || !repl->buffer || repl->editor->cursorPos == 0)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  char deletedChar = buf->value[ed->cursorPos - 1];

  memmove(&buf->value[ed->cursorPos - 1], &buf->value[ed->cursorPos],
          buf->length - ed->cursorPos + 1);

  buf->length--;
  buf->value[buf->length] = '\0';
  ed->cursorPos--;
  ed->cursorCol--;

  if (deletedChar == '\n') {
    ed->cursorLine--;
    updateCursorLineInfo(repl);
  }

  refreshDisplay(repl);
}
