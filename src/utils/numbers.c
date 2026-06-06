#include <rupa.h>

int getDouble(const char *content, int *pos) {
  if (!content || *pos == -1)
    return -1;

  bool has_comma = false;
  while (iscomma(content[*pos])) {
    has_comma = true;
    (*pos)++;
  }

  return !has_comma ? -1 : (*pos);
}

int getFloat(const char *content, int *pos) {
  if (!content || *pos == -1)
    return -1;

  bool has_dot = false;
  while (isdot(content[*pos])) {
    has_dot = true;
    (*pos)++;
  }

  return !has_dot ? -1 : (*pos);
}

int getNumber(const char *content, int *pos) {
  if (!content || *pos == -1)
    return -1;

  bool has_number = false;
  while (isdigit(content[*pos])) {
    has_number = true;
    (*pos)++;
  }

  return !has_number ? -1 : (*pos);
}
