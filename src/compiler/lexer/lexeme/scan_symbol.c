#include <rupa.h>

int scanUnderscore(Atom *atom, const char *content) {
  if (!atom || !content || atom->cursor == -1)
    return -1;

  int start = getUnderscore(content, &atom->cursor);
  atom->underscore = start != -1;

  if (!atom->underscore)
    return -1;

  if (isspace(content[start])) {
    skipWhitespace(content, &start);
  }

  char next = content[start];

  if (isalpha(next))
    return scanString(atom, content);

  else if (isdigit(next))
    return scanNumber(atom, content);

  atom->cursor = start;
  atom->has_next = nextChar(next);
  atom->next = next;
  return atom->cursor;
}
