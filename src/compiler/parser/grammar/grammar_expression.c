#include <rupa.h>

/* Kedalaman rekursi grammarParseExpr: 0 = ekspresi terluar
 * (level statement), >0 = sub-ekspresi (args, subscript, dsb). */
static int g_exprDepth = 0;

/* Wrapper memo (caches/syntax/expr.c) + pool kanonik lintas Request
 * (caches/syntax/canonical.c):
 *  - lookup per-Request → hit dalam file, gratis
 *  - adopt lintas file → salin subtree kanonik (tanpa parse)
 *  - parse → simpan memo; ekspresi terluar (depth 0) di-publish ke
 *    pool kanonik untuk dipakai file lain
 * Statement/deklarasi tidak lewat sini, jadi tidak ikut termemo. */
int grammarParseExpr(Request *r, int a, int b) {
  int cached = -1;
  if (syntaxMemoLookup(r, a, b, &cached))
    return cached;

  /* Adopt lintas file: hanya span multi-token (sub-span 1-token
   * biarkan parser jalan — overhead adopt > parse atom). */
  int adopted = syntaxCanonicalAdopt(r, a, b);
  if (adopted >= 0) {
    syntaxMemoStore(r, a, b, adopted);
    return adopted;
  }

  g_exprDepth++;
  int id = grammarParseExprBuild(r, a, b);
  g_exprDepth--;

  if (id >= 0) {
    syntaxMemoStore(r, a, b, id);
    /* Publish hanya ekspresi terluar — sub-ekspresi sudah tercakup
     * sebagai bagian subtree orang tuanya. */
    if (g_exprDepth == 0) syntaxCanonicalPublish(r, a, b, id);
  }
  return id;
}

int grammarParseExprBuild(Request *r, int a, int b) {
  while (a < b && grammarIsWhitespace(r->tokens, a))
    a++;
  while (b > a && grammarIsWhitespace(r->tokens, b - 1))
    b--;
  if (a >= b)
    return -1;
  Token *t = r->tokens;

  int asyncId = grammarParseAsyncExpr(r, a, b);
  if (asyncId != GRAMMAR_NO_MATCH)
    return asyncId;
  int awaitId = grammarParseAwaitExpr(r, a, b);
  if (awaitId != GRAMMAR_NO_MATCH)
    return awaitId;

  if (t->data[a].type == LBLOCK) {
    int id = grammarParseArrayLiteral(r, a, b);
    if (id != GRAMMAR_NO_MATCH)
      return id;
  }
  if (t->data[a].type == LBRACE) {
    int id = grammarParseObjectLiteral(r, a, b);
    if (id != GRAMMAR_NO_MATCH)
      return id;
  }
  /* IDENTIFIER / LITERAL_ID: let parseBinary handle everything.
   * parseBinary already calls grammarParsePostfixExpr and
   * grammarParseCallExpr internally, and correctly splits at binary
   * operators so that `obj.field + value` is not truncated. */
  return parseBinary(r, a, b);
}

int grammarParseArgs(Request *r, int a, int b, int **out) {
  int *ids = NULL, n = 0, start = a, d = 0;
  Token *t = r->tokens;
  for (int i = a; i <= b; i++) {
    if (i < b) {
      TokenType q = t->data[i].type;
      if (q == LPAREN || q == LBLOCK || q == LBRACE)
        d++;
      else if (q == RPAREN || q == RBLOCK || q == RBRACE)
        d--;
    }

    if (i == b || (i < b && t->data[i].type == COMMA && d == 0)) {
      int id = grammarParseExpr(r, start, i);
      if (id >= 0)
        grammarPushId(&ids, &n, id);
      start = i + 1;
    }
  }
  *out = ids;
  return n;
}
