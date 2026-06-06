#include <rupa.h>

void refreshDisplay(ReplState *repl) {
  if (!repl || !repl->editor || !repl->buffer)
    return;

  // Clear line and reposition cursor
  printf("\r\033[2K");

  Buffer *buf = repl->buffer;
  Editor *ed = repl->editor;

  if (ed->indentLevel > 0) {
    printf("%d", ed->lineNumber);

    for (int i = 0; i < ed->indentLevel + 1; i++) {
      printf("\033[1;30m-\033[0m");
    }
    // printf(" ");
  }

  else {
    printf("%d ", ed->lineNumber);
  }

  printf("%s", buf->value);
  printf("\033[%dG", getOffsetIndent(repl));
  fflush(stdout);
}
