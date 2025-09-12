#include <rupa/package.h>

TokenType gettype(const char *ptr) {
  if (strcmp(ptr, "true") == 0 || strcmp(ptr, "false") == 0)
    return BOOLEAN;

  if (strcmp(ptr, "null") == 0) {
    return NULLABLE;
  }

  if (isquote(*ptr)) {
    char quote = *ptr;
    const char *end = ptr + 1;
    while (*end && *end != quote)
      end++;
    return (*end == quote) ? STRING : UNKNOWN;
  }

  if (isint(*ptr)) {
    const char *p = ptr;
    bool has_digit = false, has_dot = false;

    while (isint(*p)) {
      has_digit = true;
      p++;
    }

    if (isdot(*p)) {
      has_dot = true;
      p++;
      bool has_frac = false;
      while (isint(*p)) {
        has_frac = true;
        p++;
      }
      if (!has_frac)
        return UNKNOWN;
    }

    return (has_digit && !has_dot) ? NUMBER : FLOAT;
  }

  if (isstr(*ptr)) {
    const char *p = ptr + 1;
    while (*p && (isstr(*p) || isint(*p)))
      p++;
    return (*p == '\0') ? IDENTIFIER : UNKNOWN;
  }

  else {
    SymbolToken *t = getSymbolToken(*ptr);
    // TokenType t = lookup_symbol(*ptr);
    if (t && t->type != UNKNOWN)
      return t->type;
  }

  return UNKNOWN;
}
