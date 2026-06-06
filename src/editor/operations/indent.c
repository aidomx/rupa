#include <rupa.h>

int findLineStart(ReplState *repl) {
  if (!repl || !repl->buffer || !repl->editor)
    return 0;

  char *buf = repl->buffer->value;
  int pos = repl->editor->cursorPos;
  for (int i = pos - 1; i >= 0; i--) {
    if (buf[i] == '\n')
      return i + 1;
  }
  return 0;
}

int getCurrentIndent(ReplState *repl) {
  if (!repl || !repl->buffer)
    return 0;

  int indent = 0;
  char *buf = repl->buffer->value;
  int length = repl->buffer->length;
  int lineStart = findLineStart(repl);
  for (int i = lineStart; i < length; i++) {
    if (isspace(buf[i])) {
      indent++;
    }

    else if (istab(buf[i])) {
      indent += 4;
    }

    else
      break;
  }

  return indent;
}

int getOffsetIndent(ReplState *repl) {
  if (!repl || !repl->editor)
    return 0;

  int baseOffset = 0;
  int lineNums = 1;
  int n = repl->editor->lineNumber;
  while (n /= 10)
    lineNums++;

  baseOffset += lineNums + 1;
  if (repl->editor->indentLevel > 0) {
    baseOffset += repl->editor->indentLevel;
  }

  return baseOffset + repl->editor->cursorPos + 1;
}

void setIndent(ReplState *repl) {
  if (!repl || !repl->buffer || !repl->editor)
    return;

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  char *ptr = buf->value;
  int length = buf->length;

  for (int i = 0; i < length; i++) {
    if (islparen(ptr[i]) || islblock(ptr[i])) {
      ed->indentLevel++;
    }

    else if (isrblock(ptr[i]) || isrparen(ptr[i])) {
      ed->indentLevel--;
      if (ed->indentLevel < 0)
        ed->indentLevel = 0;
    }
  }
}
