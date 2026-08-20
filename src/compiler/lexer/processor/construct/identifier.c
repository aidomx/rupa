#include <rupa.h>

static int word_end(const char *s, int p, int end) {
  while (p < end && (isalnum((unsigned char)s[p]) || s[p] == '_'))
    p++;
  return p;
}

static bool object_property(State *state) {
  StateContext *ctx = state ? state->context : NULL;
  if (!ctx || ctx->objectDepth <= 0 || ctx->brace < ctx->objectDepth)
    return false;

  /* Inside an object literal, an identifier followed by ':' is always a
     property separator. Type annotations belong to declarations/parameters,
     not object properties. */
  return true;
}

int processIdentifier(State *state, int start, int end, bool literal,
                      int *next) {
  if (!state || !state->input || !state->tokens || !next)
    return -1;

  const char *s = state->input->content;
  int p = word_end(s, start, end);
  if (p == start)
    return -1;

  int q = p;
  while (q < end && (s[q] == ' ' || s[q] == '\t' || s[q] == '\r'))
    q++;

  if (q < end && s[q] == ':' && !object_property(state)) {
    int t = q + 1;
    while (t < end && (s[t] == ' ' || s[t] == '\t' || s[t] == '\r'))
      t++;
    int te = word_end(s, t, end);
    if (te == t)
      return -1;

    char *id = substring(s, start, p);
    char *type = substring(s, t, te);
    if (!id || !type) {
      gcfree(id);
      gcfree(type);
      return -1;
    }

    addToken(state->tokens,
             createDataToken(id, type, literal ? LITERAL_ID : IDENTIFIER,
                             state->input->line, start));
    state->input->flags->isAnnotionType = true;
    gcfree(id);
    gcfree(type);
    *next = te;
    return 0;
  }

  char *value = substring(s, start, p);
  if (!value)
    return -1;
  TokenType type = gettype(value);
  if (literal && type == IDENTIFIER)
    type = LITERAL_ID;
  addToken(state->tokens,
           createDataToken(value, NULL, type, state->input->line, start));
  gcfree(value);
  *next = p;
  return 0;
}
