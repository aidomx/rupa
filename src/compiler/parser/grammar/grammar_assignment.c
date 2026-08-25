#include <rupa.h>

int grammarParseAssignment(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;

  for (int i = a + 1; i < b; i++)
    if (t->data[i].type == ASSIGN) {
      int l = grammarParseExpr(r, a, i);
      int rr = grammarParseExpr(r, i + 1, b);
      int type = -1;

      if (t->data[a].type == IDENTIFIER && t->data[a].safetyType)
        type = createId(r->node, t->data[a].safetyType);

      *pos = b;
      if (l >= 0 && rr >= 0)
        return createAssignment(r->node, l, type, rr);
      return -1;
    }

  return GRAMMAR_NO_MATCH;
}

int grammarParseConditionalAssignment(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  for (int i = a + 1; i < b; i++) {
    if (t->data[i].type != CONDITIONAL_ASSIGN)
      continue;
    int target = grammarParseExpr(r, a, i);
    int value = grammarParseExpr(r, i + 1, b);
    *pos = b;
    if (target >= 0 && value >= 0)
      return createConditionalAssignment(r->node, target, value);
    return -1;
  }
  return GRAMMAR_NO_MATCH;
}

int grammarParseExpressionStatement(Request *r, int a, int b, int *pos) {
  int e = grammarParseExpr(r, a, b);
  *pos = b;
  return e >= 0 ? createReturn(r->node, e) : -1;
}
