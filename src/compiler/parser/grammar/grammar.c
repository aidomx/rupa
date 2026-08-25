#include <rupa.h>

/*
 * Titik masuk utama grammar bahasa Rupa. Urutan pengecekan di bawah ini
 * SENGAJA dipertahankan sama persis dengan urutan grammar aslinya (lihat
 * riwayat processor.c): keyword lebih dulu, lalu function/struct
 * declaration, lalu annotation, lalu assignment, dan fallback ekspresi
 * paling akhir. Menambah grammar baru berarti menambah satu unit
 * grammar_*.c baru (lihat grammar.h) dan mendaftarkan pemanggilannya di
 * sini, tanpa perlu menyentuh unit grammar lain.
 */
int grammarParseStatement(Request *r, int *pos, int limit) {
  Token *t = r->tokens;
  while (*pos < limit && grammarIsWhitespace(t, *pos))
    (*pos)++;
  if (*pos >= limit)
    return -1;
  int a = *pos, b = grammarLineEnd(t, a);
  if (b > limit)
    b = limit;

  /* case may be emitted as a keyword by the processor; keep the grammar
   * check value-based as a defensive path while keyword tables evolve. */
  if (!strcmp(t->data[a].value, "case")) {
    int id = grammarParseCase(r, a, b, limit, pos);
    if (id != GRAMMAR_NO_MATCH) return id;
  }

  if (t->data[a].type == KEYWORD) {
    int id;
    /* async is an expression grammar, but at statement level it must stay
     * a standalone Async node rather than falling through expression-statement
     * wrapping (which currently creates a Return node). */
    if ((id = grammarParseAsyncExpr(r, a, b)) != GRAMMAR_NO_MATCH) {
      *pos = b;
      return id;
    }
    if ((id = grammarParseReturnKeyword(r, a, b, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParseControl(r, a, b, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParsePrint(r, a, b, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParseCase(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParseIf(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParseLoop(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParseModule(r, a, b, pos)) != GRAMMAR_NO_MATCH)
      return id;
  }

  int id;
  if ((id = grammarParseFunction(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH)
    return id;
  if ((id = grammarParseStruct(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH)
    return id;
  if ((id = grammarParseAnnotation(r, a, b, pos)) != GRAMMAR_NO_MATCH)
    return id;
  if ((id = grammarParseUpdate(r, a, b, pos)) != GRAMMAR_NO_MATCH)
    return id;
  if ((id = grammarParseConditionalAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH)
    return id;
  if ((id = grammarParseAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH)
    return id;

  return grammarParseExpressionStatement(r, a, b, pos);
}
