#include <rupa.h>

void annotationType(const char *buffer, int *position) {
  skipWhitespace(buffer, position);
  while (isalpha(buffer[*position]) || isunderscore(buffer[*position]))
    (*position)++;
  skipWhitespace(buffer, position);
}

int braces(const char *buffer, int start, int *level) {
  if (!buffer || start == -1 || *level == -1)
    return -1;

  int i = start;

  for (; buffer[i]; i++) {
    if (islblock(buffer[i]))
      (*level)++;

    else if (isrblock(buffer[i])) {
      (*level)--;

      if (*level == 0)
        return i + 1;
    }
  }

  return i;
}

int consumeToEnd(const char *buffer, int pos) {
  if (!buffer || pos == -1)
    return -1;

  if (buffer[pos] == '\0')
    return pos;

  while (buffer[pos])
    pos++;
  return pos;
}

int parenthesis(const char *buffer, int start, int *level) {
  if (!buffer || start == -1 || *level == -1)
    return -1;

  int i = start;

  for (; buffer[i]; i++) {
    if (islparen(buffer[i]))
      (*level)++;

    else if (isrparen(buffer[i])) {
      (*level)--;

      if (*level == 0)
        return i + 1;
    }
  }

  return i;
}

void skipWhitespace(const char *buffer, int *position) {
  while (isspace(buffer[*position]))
    (*position)++;
}

void subscripts(const char *buffer, int *position) {
  while (!isrbracket(buffer[*position]))
    (*position)++;
}
