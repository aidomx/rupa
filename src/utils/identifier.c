#include <rupa.h>

typedef struct {
  bool has_number;
  bool has_string;
  bool has_underscore;
  bool has_next_id;
} Ident;

void scanIdentifier(const char *buffer, Ident *id, int *pos) {
  if (!buffer || *pos == -1)
    return;

  char first = buffer[*pos];

  // first character of identifier is string
  if (isalpha(first)) {
    int start = getString(buffer, pos);
    id->has_string = start != -1;

    if (!id->has_string)
      return;

    char second = buffer[start];

    // ex: x1
    if (isdigit(second)) {
      int end = getNumber(buffer, &start);
      id->has_number = end != -1;

      if (!id->has_number)
        return;

      // ex: x1_
      char three = buffer[end];
      if (isunderscore(three)) {
        int last = getUnderscore(buffer, &end);
        id->has_underscore = last != -1;

        if (!id->has_underscore)
          return;

        // ex: x1_,
        if (nextChar(buffer[last])) {
          id->has_next_id = true;
          *pos = last;
        }

        else
          *pos = last;
      }

      // ex: x1,
      else if (nextChar(three))
        *pos = end;

      // ex: x1
      else
        *pos = end;
    }

    // ex: x_
    else if (isunderscore(second)) {
      int end = getUnderscore(buffer, &start);
      id->has_underscore = end != -1;

      if (!id->has_underscore)
        return;

      // ex: x_1
      char three = buffer[end];
      if (isdigit(three)) {
        int last = getNumber(buffer, &end);
        id->has_number = last != -1;

        if (!id->has_number)
          return;

        // ex: x_1,
        if (nextChar(buffer[last])) {
          id->has_next_id = true;
          *pos = last;
        }

        // ex: x_1
        else
          *pos = last;
      }

      // ex: x_,
      else if (nextChar(three)) {
        id->has_next_id = true;
        *pos = end;
      }

      // ex: x_
      else
        *pos = end;
    }

    // ex: x,
    else if (nextChar(second)) {
      id->has_next_id = true;
      *pos = start;
    }

    // ex: x
    else
      *pos = start;
  }

  // first character of identifier is number
  else if (isdigit(first)) {
    int start = getNumber(buffer, pos);
    id->has_number = start != -1;

    if (!id->has_number)
      return;

    char next = buffer[start];

    // ex: 1a  -> invalid identifier (number followed by alpha)
    if (isalpha(next) || isunderscore(next)) {
      id->has_string = isalpha(next);
      id->has_underscore = isunderscore(next);
      *pos = start;
      return;
    }

    // ex: 123,
    if (iscomma(next) || isdot(next)) {
      while (iscomma(buffer[start]) || isdot(buffer[start]))
        start++;

      if (!isdigit(buffer[start])) {
        *pos = start;
      }

      else {
        *pos = getNumber(buffer, &start);
      }
    }

    // ex: 123
    else {
      *pos = start;
    }
  }

  // first character of identifier is underscore
  else if (isunderscore(first)) {
    int start = getUnderscore(buffer, pos);
    id->has_underscore = start != -1;

    if (!id->has_underscore)
      return;

    char second = buffer[start];

    // ex: _x
    if (isalpha(second)) {
      int end = getString(buffer, &start);
      id->has_string = end != -1;

      if (!id->has_string)
        return;

      char third = buffer[end];

      // ex: _x1
      if (isdigit(third)) {
        int last = getNumber(buffer, &end);
        id->has_number = last != -1;

        if (!id->has_number)
          return;

        if (nextChar(buffer[last])) {
          id->has_next_id = true;
          *pos = last;
        } else {
          *pos = last;
        }
      }

      // ex: _x,
      else if (nextChar(third)) {
        id->has_next_id = true;
        *pos = end;
      }

      // ex: _x
      else {
        *pos = end;
      }
    }

    // ex: _1  → invalid identifier
    else if (isdigit(second)) {
      id->has_number = true;
      *pos = start;
      return;
    }

    // ex: _,
    else if (nextChar(second)) {
      id->has_next_id = true;
      *pos = start;
    }

    // ex: _
    else {
      *pos = start;
    }
  }
}

int scanId(Atom *atom) {
  if (!atom || !atom->content || atom->cursor == -1)
    return -1;

  atom->prev = atom->next;

  const char *content = atom->content;
  int start = getString(content, &atom->cursor);
  atom->string = start != -1;

  if (!atom->string)
    return -1;

  char second = content[start];

  // ex: x1
  if (isdigit(second)) {
    int end = getNumber(content, &start);
    atom->number = end != -1;

    if (!atom->number)
      return start;

    // ex: x1_
    char three = content[end];
    if (isunderscore(three)) {
      int last = getUnderscore(content, &end);
      atom->underscore = last != -1;

      if (!atom->underscore)
        return end;

      // ex: x1_,
      if (nextChar(content[last])) {
        atom->next = content[last];
        atom->cursor = last;
      }

      else
        atom->cursor = last;
    }

    // ex: x1,
    else if (nextChar(three))
      atom->cursor = end;

    // ex: x1
    else
      atom->cursor = end;
  }

  // ex: x_
  else if (isunderscore(second)) {
    int end = getUnderscore(content, &start);
    atom->underscore = end != -1;

    if (!atom->underscore)
      return end;

    // ex: x_1
    char three = content[end];
    if (isdigit(three)) {
      int last = getNumber(content, &end);
      atom->number = last != -1;

      if (!atom->number)
        return last;

      // ex: x_1,
      if (nextChar(content[last])) {
        atom->next = content[last];
        atom->cursor = last;
      }

      // ex: x_1
      else
        atom->cursor = last;
    }

    // ex: x_,
    else if (nextChar(three)) {
      atom->next = three;
      atom->cursor = end;
    }

    // ex: x_
    else
      atom->cursor = end;
  }

  // ex: x,
  else if (nextChar(second)) {
    atom->next = second;
    atom->cursor = start;
  }

  // ex: x
  else
    atom->cursor = start;

  return atom->cursor;
}

int getIdentifier(const char *content, int *pos) {
  if (!content || *pos == -1)
    return -1;

  Ident id = {.has_next_id = false,
              .has_number = false,
              .has_string = false,
              .has_underscore = false};

  int start = (*pos);
  skipWhitespace(content, pos);
  // scanId(content, &id, pos);
  skipWhitespace(content, pos);
  int end = (*pos);

  bool is_number = (id.has_number && !id.has_string && !id.has_underscore);

  // after identifier
  // ex: x(, x[, x{, x:, x=, and [x[operator]]
  if (!id.has_next_id)
    return !is_number ? (*pos) : -1;

  char word[512];
  int length = 0;
  for (int i = start; i < end; i++) {
    word[length++] = content[i];
  }

  if (length == 0)
    return -1;

  word[length] = '\0';
  bool is_boolean = (strcmp(word, "true") == 0 || strcmp(word, "false") == 0);

  return !is_boolean ? (*pos) : -1;
}

bool nextChar(char c) { return next_id(c); }
