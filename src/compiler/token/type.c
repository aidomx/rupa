#include <rupa.h>

typedef struct {
  bool separator;
  bool has_comma;
  bool has_dot;
  bool has_number;
  bool has_string;
  bool has_underscore;
  bool next_is_number;
  int count_comma;
  int count_dot;
} Candidate;

Candidate *createCandidate(void) {
  Candidate *c = gcmall(sizeof(Candidate));
  memset(c, 0, sizeof(Candidate));
  return c;
}

Candidate *getCandidate(const char *ptr) {
  if (!ptr)
    return NULL;

  Candidate *candidate = createCandidate();

  while (*ptr &&
         (candidate_number(*ptr) || isalpha(*ptr) || isunderscore(*ptr))) {
    char c = *ptr;

    if (isalpha(c))
      candidate->has_string = true;

    if (iscomma(c)) {
      candidate->count_comma++;
      candidate->has_comma = true;
    }

    if (isdigit(c))
      candidate->has_number = true;

    else if (isdot(c)) {
      candidate->count_dot++;
      candidate->has_dot = true;
    }

    else if (isunderscore(c)) {
      candidate->has_underscore = true;
    }

    if (iscomma(c) || isdot(c))
      candidate->separator = true;

    else if (isdigit(c) && candidate->separator)
      candidate->next_is_number = true;

    ptr++;
  }

  return candidate;
}

bool isCandidateId(Candidate *c) {
  if (!c)
    return false;

  if (c->has_comma || c->has_dot)
    return false;

  return c->has_string || c->has_underscore;
}

bool isCandidateDouble(Candidate *c) {
  if (!c)
    return false;

  bool has_comma = c->has_comma, has_number = c->has_number,
       next_is_number = c->next_is_number;
  int depth_comma = c->count_comma;

  if (depth_comma > 1)
    return false;

  return (has_number && has_comma && next_is_number);
}

bool isCandidateFloat(Candidate *c) {
  if (!c)
    return false;

  bool has_dot = c->has_dot, has_number = c->has_number,
       next_is_number = c->next_is_number;
  int depth_dot = c->count_dot;

  if (depth_dot > 1)
    return false;

  return (has_number && has_dot && next_is_number);
}

bool isCandidateNumber(Candidate *c) {
  if (!c)
    return false;

  bool has_comma = c->has_comma, has_dot = c->has_dot,
       has_number = c->has_number, has_string = c->has_string,
       has_underscore = c->has_underscore;

  return (has_number && !has_comma && !has_dot && !has_string &&
          !has_underscore);
}

TokenType isNumberOrIdent(const char *ptr) {
  if (!ptr)
    return UNKNOWN;

  Candidate *c = getCandidate(ptr);

  bool is_double = isCandidateDouble(c);
  bool is_identifier = isCandidateId(c);
  bool is_float = isCandidateFloat(c);
  bool is_number = isCandidateNumber(c);

  return is_double       ? DOUBLE
         : is_float      ? FLOAT
         : is_identifier ? IDENTIFIER
         : is_number     ? NUMBER
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

    return (has_digit && !has_dot) ? NUMBER : FLOAT;
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
