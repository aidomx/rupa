#include <rupa.h>

/*
 * Grammar update untuk increment/decrement dan compound assignment.
 *
 * Postfix: identifier++ / identifier--
 * Prefix : ++identifier / --identifier
 * Compound: identifier += expr, -=, *=, /=, %=
 *
 * Update sengaja menjadi statement sendiri, bukan binary expression,
 * karena interpreter nantinya perlu mengetahui efek samping dan urutan
 * evaluasi prefix/postfix. Compound menyimpan operand kanan pada
 * update.value (node id) dan dievaluasi sebagai x OP v oleh runtime.
 */
int grammarParseUpdate(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  int target = -1;
  const char *op = NULL;
  bool prefix = false;
  int value = -1;

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
    /* Compound assignment: target += value (juga -= *= /= %=) */
    int opPos = -1;
    for (int i = a + 1; i < b; i++) {
      switch (t->data[i].type) {
      case PLUS_ASSIGN:
      case MINUS_ASSIGN:
      case STAR_ASSIGN:
      case SLASH_ASSIGN:
      case PERCENT_ASSIGN:
        opPos = i;
        break;
      default:
        break;
      }
      if (opPos >= 0) break;
    }
    if (opPos < 0 || opPos != a + 1)
      return GRAMMAR_NO_MATCH;

    switch (t->data[opPos].type) {
    case PLUS_ASSIGN: op = "+="; break;
    case MINUS_ASSIGN: op = "-="; break;
    case STAR_ASSIGN: op = "*="; break;
    case SLASH_ASSIGN: op = "/="; break;
    case PERCENT_ASSIGN: op = "%="; break;
    default: return GRAMMAR_NO_MATCH;
    }

    target = grammarParseExpr(r, a, opPos);
    value = grammarParseExpr(r, opPos + 1, b);
    if (target < 0 || value < 0)
      return -1;

    *pos = b;
    return createUpdate(r->node, target, op, false, value);
  }

  if (target < 0)
    return -1;

  *pos = b;
  return createUpdate(r->node, target, op, prefix, -1);
}
