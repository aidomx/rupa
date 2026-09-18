#include <rupa.h>

int grammarParseCallExpr(Request *r, int a, int b) {
  Token *t = r->tokens;
  if (a + 1 >= b)
    return GRAMMAR_NO_MATCH;

  /* new T(args) — alokasi type-driven (design/new_memory.txt):
   * callee `new` + NAMA TIPE sebagai arg[0] (tidak dievaluasi).
   * Kapitalisasi penting: `new Number()`, bukan `new number()`. */
  if ((t->data[a].type == IDENTIFIER || t->data[a].type == LITERAL_ID) &&
      t->data[a].value && !strcmp(t->data[a].value, "new") && a + 2 < b &&
      (t->data[a + 1].type == IDENTIFIER || t->data[a + 1].type == LITERAL_ID) &&
      t->data[a + 2].type == LPAREN) {
    int c = grammarMatchClose(t, a + 2, b, LPAREN, RPAREN);
    if (c != b - 1)
      return GRAMMAR_NO_MATCH;
    int callee = createId(r->node, "new");
    int typeNode = createId(r->node, t->data[a + 1].value);
    int *as = NULL, n = grammarParseArgs(r, a + 3, c, &as);
    int *args = gcmall(sizeof(int) * (size_t)(n + 1));
    if (!args)
      return -1;
    args[0] = typeNode;
    for (int i = 0; i < n; i++)
      args[i + 1] = as ? as[i] : -1;
    return createCall(r->node, callee, args, n + 1);
  }

  if (t->data[a + 1].type != LPAREN)
    return GRAMMAR_NO_MATCH;

  int c = grammarMatchClose(t, a + 1, b, LPAREN, RPAREN);
  if (c != b - 1)
    return GRAMMAR_NO_MATCH;

  int callee = createId(r->node, t->data[a].value);
  int *as = NULL, n = grammarParseArgs(r, a + 2, c, &as);
  return createCall(r->node, callee, as, n);
}
