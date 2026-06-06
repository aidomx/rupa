#include <rupa.h>

int scanNumber(Atom *atom, const char *content) {
  if (!atom || !content || atom->cursor == -1)
    return -1;

  int start = getNumber(content, &atom->cursor);
  atom->number = start != -1;

  if (!atom->number)
    return -1;

  if (isspace(content[start])) {
    skipWhitespace(content, &start);
  }

  char next = content[start];

  // ex: 1x
  if (isalpha(next))
    return scanString(atom, content);

  // ex: 1_
  else if (isunderscore(next))
    return scanUnderscore(atom, content);

  atom->cursor = start;
  atom->has_next = iscomma(next) || isdot(next);
  atom->next = next;
  return atom->cursor;
}
