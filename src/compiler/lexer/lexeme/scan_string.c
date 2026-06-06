#include <rupa.h>

int scanString(Atom *atom, const char *content) {
  if (!atom || !content || atom->cursor == -1)
    return -1;

  int start = getString(content, &atom->cursor);
  atom->string = start != -1;

  if (!atom->string)
    return -1;

  if (isspace(content[start])) {
    skipWhitespace(content, &start);
  }

  char next = content[start];

  // ex: x1
  if (isdigit(next))
    return scanNumber(atom, content);

  // ex: x_
  else if (isunderscore(next))
    return scanUnderscore(atom, content);

  atom->cursor = start;
  atom->has_next = nextChar(next);
  atom->next = next;
  return atom->cursor;
}
