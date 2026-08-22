#include <rupa.h>

/*
 * break/continue adalah control statement tanpa operand.
 * Jika keyword cocok tetapi ada token lain pada statement yang sama, parse gagal
 * agar `continue x` tidak diam-diam diterima sebagai continue biasa.
 */
int grammarParseControl(Request *r, int a, int b, int *pos) {
  if (!r || !r->tokens || a < 0 || a >= b)
    return GRAMMAR_NO_MATCH;

  Token *t = r->tokens;
  if (t->data[a].type != KEYWORD)
    return GRAMMAR_NO_MATCH;

  const char *k = t->data[a].value;
  if (strcmp(k, "break") && strcmp(k, "continue"))
    return GRAMMAR_NO_MATCH;

  if (a + 1 != b)
    return -1;

  *pos = b;
  return !strcmp(k, "break") ? createBreak(r->node) : createContinue(r->node);
}
