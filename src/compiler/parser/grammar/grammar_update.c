#include <rupa.h>

/*
 * Grammar update untuk increment/decrement.
 *
 * Postfix: identifier++ / identifier--
 * Prefix : ++identifier / --identifier
 *
 * Update sengaja menjadi statement sendiri, bukan binary expression,
 * karena interpreter nantinya perlu mengetahui efek samping dan urutan
 * evaluasi prefix/postfix.
 */
int grammarParseUpdate(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  int target = -1;
  const char *op = NULL;
  bool prefix = false;

  if (b - a == 2 &&
      (t->data[a + 1].type == INCREMENT || t->data[a + 1].type == DECREMENT)) {
    target = grammarParseExpr(r, a, a + 1);
    op = t->data[a + 1].type == INCREMENT ? "++" : "--";
  } else if (b - a == 2 &&
             (t->data[a].type == INCREMENT || t->data[a].type == DECREMENT)) {
    target = grammarParseExpr(r, a + 1, b);
    op = t->data[a].type == INCREMENT ? "++" : "--";
    prefix = true;
  } else {
    return GRAMMAR_NO_MATCH;
  }

  if (target < 0)
    return -1;

  *pos = b;
  return createUpdate(r->node, target, op, prefix);
}
