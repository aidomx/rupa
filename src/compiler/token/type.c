#include <rupa.h>

typedef struct {
  bool has_dot;
  bool has_number;
  bool has_string;
  bool has_underscore;
  bool digit_after_dot;
  int count_dot;
} Candidate;

static Candidate getCandidate(const char *ptr) {
  Candidate candidate = {0};
  bool after_dot = false;

  if (!ptr)
    return candidate;

  while (*ptr && (isalnum((unsigned char)*ptr) || isunderscore(*ptr) ||
                  isdot(*ptr))) {
    char c = *ptr++;

    if (isalpha((unsigned char)c))
      candidate.has_string = true;
    else if (isdigit((unsigned char)c)) {
      candidate.has_number = true;
      if (after_dot)
        candidate.digit_after_dot = true;
    } else if (isdot(c)) {
      candidate.has_dot = true;
      candidate.count_dot++;
      after_dot = true;
      continue;
    } else if (isunderscore(c)) {
      candidate.has_underscore = true;
    }
  }

  return candidate;
}

static bool isCandidateId(const Candidate *c) {
  return c && !c->has_dot && (c->has_string || c->has_underscore);
}

static bool isCandidateDecimal(const Candidate *c) {
  return c && c->has_number && c->has_dot && c->count_dot == 1 &&
         c->digit_after_dot && !c->has_string && !c->has_underscore;
}

static bool isCandidateNumber(const Candidate *c) {
  return c && c->has_number && !c->has_dot && !c->has_string &&
         !c->has_underscore;
}

TokenType isNumberOrIdent(const char *ptr) {
  if (!ptr)
    return UNKNOWN;

  Candidate c = getCandidate(ptr);

  return isCandidateDecimal(&c) ? DECIMAL
         : isCandidateId(&c)    ? IDENTIFIER
         : isCandidateNumber(&c) ? NUMBER
                                 : UNKNOWN;
}

TokenType setTokenType(const char *ptr) {
  if (!ptr)
    return UNKNOWN;

  if (strcmp(ptr, "true") == 0 || strcmp(ptr, "false") == 0)
    return BOOLEAN;

  if (strcmp(ptr, "null") == 0)
    return NULLABLE;

  return (isalpha(*ptr) || isdigit(*ptr) || isunderscore(*ptr))
             ? isNumberOrIdent(ptr)
             : UNKNOWN;
}

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

    return (has_digit && !has_dot) ? NUMBER : DECIMAL;
  }

  if (isstr(*ptr) || isunderscore(*ptr)) {
    const char *p = ptr + 1;
    while (*p && (isstr(*p) || isint(*p) || isunderscore(*p)))
      p++;
    return (*p == '\0') ? IDENTIFIER : UNKNOWN;
  }

  else {
    Symbol *symbol = getSymbolToken(*ptr);
    // TokenType t = lookup_symbol(*ptr);
    if (symbol && symbol->type != UNKNOWN)
      return symbol->type;
  }

  return UNKNOWN;
}
