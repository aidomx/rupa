#include <rupa.h>

int grammarParseFunction(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;

  // name(...) { } => function, name(...) => call
  if (t->data[a].type != IDENTIFIER || a + 1 >= b ||
      t->data[a + 1].type != LPAREN)
    return GRAMMAR_NO_MATCH;

  int c = grammarMatchClose(t, a + 1, limit, LPAREN, RPAREN);
  if (c <= 0)
    return GRAMMAR_NO_MATCH;

  int name = createId(r->node, t->data[a].value);
  int *ps = NULL, n = 0, start = a + 2;
  int depth = 0; /* kedalaman kurung: koma di dalam nested call bukan pemisah */
  for (int j = a + 2; j <= c; j++) {
    TokenType q = t->data[j].type;
    if (q == LPAREN || q == LBLOCK || q == LBRACE)
      depth++;
    else if (q == RPAREN || q == RBLOCK || q == RBRACE)
      depth--;
    if (j == c || (j < c && depth == 0 && t->data[j].type == COMMA)) {
      int pid = -1;

      /* The lexer normalizes a simple "name: Type" parameter into a
       * single token (IDENTIFIER/LITERAL_ID with .safetyType set), same as
       * it does for "x: Type = value" assignments/annotations. Read that
       * first, the same way grammarParseAnnotation() does; without this,
       * the literal-COLON scan below never finds a COLON token to split
       * on (there isn't one), so the parameter's type was silently
       * dropped. */
      pid = grammarAnnotationFromSafetyType(r, start, -1);
      if (pid < 0) {
        int sep = -1;
        for (int q = start; q < j; q++)
          if (t->data[q].type == COLON) {
            sep = q;
            break;
          }
        if (sep >= 0) {
          int pn = grammarParseExpr(r, start, sep);
          int pt = grammarParseExpr(r, sep + 1, j);
          /* Explicit colon-form parameters may also use array types. */
          if (sep + 2 < j && t->data[sep + 2].type == IDENTIFIER) {
            int candidate = sep + 2;
            if (candidate + 1 == j || t->data[candidate + 1].type == LBLOCK)
              pt = createTypeNode(r->node, t->data[candidate].value);
          }
          pid = createAnnotation(r->node, pn, pt, -1);
        } else {
          pid = grammarParseExpr(r, start, j);
        }
      }

      if (pid >= 0)
        grammarPushId(&ps, &n, pid);
      start = j + 1;
    }
  }

  /* Return-type annotation (design/rupa_types_const_void_bigint.txt poin 3):
   * `foo(params): void { }` / `foo(params): number { }` — colon setelah
   * ')'. Lexer sudah menandai posisinya via flags->isReturnType dan TIDAK
   * memperlakukannya sebagai colon-body, jadi token ':' + nama tipe ada
   * di sini antara ')' dan '{'. */
  int returnType = -1;
  int head = c + 1;
  if (head < limit && t->data[head].type == COLON) {
    int typeStart = head + 1;
    /* Tipe boleh punya postfix array: `foo(): number[] { }` — lexer
     * normalizer tidak jalan di posisi ini (tidak ada `name: Type`
     * statement), jadi scan manual: word + nol atau lebih `[]`. */
    if (typeStart < limit && (t->data[typeStart].type == IDENTIFIER ||
                              t->data[typeStart].type == LITERAL_ID ||
                              t->data[typeStart].type == KEYWORD)) {
      int typeEnd = typeStart;
      while (typeEnd + 1 < limit && t->data[typeEnd + 1].type == LBLOCK &&
             typeEnd + 2 < limit && t->data[typeEnd + 2].type == RBLOCK)
        typeEnd += 2;
      int typeId = createTypeNode(r->node, t->data[typeStart].value);
      for (int q = typeStart + 1; q <= typeEnd && typeId >= 0; q += 2)
        typeId = createArrayType(r->node, typeId);
      returnType = typeId;
      head = typeEnd + 1;
    }
  }

  if (head < limit && t->data[head].type == LBRACE) {
    int close = grammarMatchClose(t, head, limit, LBRACE, RBRACE);
    int body = close >= 0 ? grammarParseBlock(r, head, close) : -1;
    int id = createFunctionDecl(r->node, name, ps, n, body, returnType);
    *pos = close >= 0 ? close + 1 : b;
    return id;
  }

  /* `foo(params): Type` tanpa body + tanpa call-arg = deklarasi function
   * dengan return type saja (tanpa body) — bukan call. */
  if (returnType >= 0) {
    int id = createFunctionDecl(r->node, name, ps, n, -1, returnType);
    *pos = head;
    return id;
  }

  int id = createCall(r->node, name, ps, n);
  *pos = b;
  return id;
}
