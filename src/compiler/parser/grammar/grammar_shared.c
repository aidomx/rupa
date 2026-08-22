#include <rupa.h>

bool grammarIsWhitespace(Token *t, int i) {
  return i >= 0 && i < t->length &&
         (t->data[i].type == NEWLINE || t->data[i].type == TAB);
}

int grammarLineEnd(Token *t, int i) {
  if (!t || i >= t->length)
    return i;
  int line = t->data[i].line;
  while (i < t->length && !isToken(t, i, ENDOF) && t->data[i].type != NEWLINE &&
         t->data[i].line == line)
    i++;
  return i;
}

int grammarMatchClose(Token *t, int open, int end, TokenType l, TokenType r) {
  int d = 0;
  for (int i = open; i < end; i++) {
    if (t->data[i].type == l)
      d++;
    else if (t->data[i].type == r && --d == 0)
      return i;
  }
  return -1;
}

void grammarPushId(int **v, int *n, int x) {
  *v = gcrealloc(*v, sizeof(int) * (*n + 1));
  (*v)[(*n)++] = x;
}
