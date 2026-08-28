#include <rupa.h>

int grammarParseAssignment(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;

  /* Find the ASSIGN token at depth 0 to avoid matching '=' inside nested
   * parentheses/brackets/braces. */
  int assignPos = -1;
  for (int i = a + 1; i < b; i++) {
    if (t->data[i].type == ASSIGN) {
      assignPos = i;
      break;
    }
  }
  if (assignPos < 0) return GRAMMAR_NO_MATCH;

  int rr = grammarParseExpr(r, assignPos + 1, b);
  if (rr < 0) return -1;

  /* Simple identifier target: name = expr */
  if (assignPos == a + 1 &&
      (t->data[a].type == IDENTIFIER || t->data[a].type == LITERAL_ID)) {
    int l = createId(r->node, t->data[a].value);
    int type = -1;
    if (t->data[a].type == IDENTIFIER && t->data[a].safetyType)
      type = createId(r->node, t->data[a].safetyType);
    *pos = b;
    if (l >= 0)
      return createAssignment(r->node, l, type, rr);
    return -1;
  }

  /* Member / subscript target: obj.field = expr, arr[i] = expr */
  int target = grammarParsePostfixExpr(r, a, assignPos);
  if (target >= 0) {
    *pos = b;
    return createMemberAssign(r->node, target, rr);
  }

  return GRAMMAR_NO_MATCH;
}

int grammarParseConditionalAssignment(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  for (int i = a + 1; i < b; i++) {
    if (t->data[i].type != CONDITIONAL_ASSIGN)
      continue;
    int target = (i == a + 1 &&
                  (t->data[a].type == IDENTIFIER || t->data[a].type == LITERAL_ID))
                     ? createId(r->node, t->data[a].value)
                     : -1;
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
