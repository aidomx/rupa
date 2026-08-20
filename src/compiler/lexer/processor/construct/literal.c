#include <rupa.h>

static int line_of(State *s) { return s->input->line; }

int processString(State *state, int start, int end, int *next, bool *waiting) {
  const char *s = state->input->content;
  char quote = s[start];
  bool escaped = false;
  int p = start + 1;

  while (p < end) {
    char c = s[p++];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == quote) {
      char *value = substring(s, start, p);
      if (!value)
        return -1;
      addToken(state->tokens,
               createDataToken(value, NULL, STRING, line_of(state), start));
      gcfree(value);
      *next = p;
      *waiting = false;
      return 0;
    }
  }

  *waiting = true;
  return -1;
}

int processNumber(State *state, int start, int end, int *next, bool *waiting) {
  const char *s = state->input->content;
  int p = start;
  bool dot = false;
  bool digitAfterDot = false;

  while (p < end && isdigit((unsigned char)s[p]))
    p++;

  if (p < end && s[p] == '.') {
    dot = true;
    p++;
    while (p < end && isdigit((unsigned char)s[p])) {
      digitAfterDot = true;
      p++;
    }
    if (!digitAfterDot) {
      if (p >= end) {
        *waiting = true;
        return -1;
      }
      return -1;
    }
  }

  if (p < end && (isalpha((unsigned char)s[p]) || s[p] == '_'))
    return -1;

  char *value = substring(s, start, p);
  if (!value)
    return -1;
  TokenType type = dot ? FLOAT : NUMBER;
  addToken(state->tokens,
           createDataToken(value, NULL, type, line_of(state), start));
  gcfree(value);
  *next = p;
  *waiting = false;
  return 0;
}
