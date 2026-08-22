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

  if (t->data[a].type == KEYWORD) {
    int id;
    if ((id = grammarParseReturnKeyword(r, a, b, pos)) != GRAMMAR_NO_MATCH)
      return id;
    if ((id = grammarParsePrint(r, a, b, pos)) != GRAMMAR_NO_MATCH)
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
  if ((id = grammarParseAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH)
    return id;

  return grammarParseExpressionStatement(r, a, b, pos);
}
