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
/* Statement separator satu baris (design/variable.txt): `x = 1; y = 2`
 * dan `x = 1, y = 2` = dua statement. Cari SEMICOLON/COMMA top-level
 * (di luar kurung/kurawal/siku) yang TIDAK diakhiri baris yang sama —
 * `,` di akhir baris diperlakukan whitespace. Return posisi separator,
 * atau -1.
 *
 * Keyword TIDAK di-split: `for i=0; i<10` memakai `;` di header loop,
 * `import a, b from ./m` memakai `,` di daftar entry (keduanya punya
 * grammar sendiri yang mengonsumsi separator tsb). `case`/`enum` juga
 * aman: member dipisah NEWLINE/COMMA di dalam block grammar masing-
 * masing, bukan statement. */
static int grammarStatementSplit(Token *t, int a, int b) {
  int depth = 0;
  for (int i = a; i < b; i++) {
    TokenType type = t->data[i].type;
    if (type == LPAREN || type == LBRACE || type == LBLOCK)
      depth++;
    else if (type == RPAREN || type == RBRACE || type == RBLOCK)
      depth--;
    else if (depth == 0 && (type == SEMICOLON || type == COMMA)) {
      /* `,` atau `;` di akhir baris: terminator kosong, bukan pemisah
       * statement baru. */
      int j = i + 1;
      while (j < b && grammarIsWhitespace(t, j)) j++;
      if (j >= b)
        continue;
      return i;
  }
  }
  return -1;
}

int grammarParseStatement(Request *r, int *pos, int limit) {
  Token *t = r->tokens;
  while (*pos < limit && grammarIsWhitespace(t, *pos))
    (*pos)++;
  if (*pos >= limit) return -1;

  int a = *pos, b = grammarLineEnd(t, a);
  g_parser_token = &t->data[a];
  if (b > limit) b = limit;

  /* Separator statement satu baris: parse bagian sebelum `;`/`,` lalu
   * lanjutkan SETELAHNYA (baris sama, statement berikutnya). Statement
   * ber-keyword dilewati: loop memakai `;` di header (`for i=0; i<10`),
   * import/export memakai `,` di daftar entry — grammar mereka sendiri
   * yang mengonsumsi separator tersebut. */
  int sep = (t->data[a].type == KEYWORD) ? -1 : grammarStatementSplit(t, a, b);
  if (sep > a) {
    int save = b;
    b = sep;
    int id = grammarParseStatementBody(r, a, b, limit, pos);
    if (id != GRAMMAR_NO_MATCH) {
      /* Maju melewati separator (dan whitespace) — *pos menunjuk awal
       * statement berikutnya di baris yang sama. */
      int p = sep + 1;
      while (p < limit && grammarIsWhitespace(t, p)) p++;
      *pos = p;
      return id;
    }
    b = save;
  }

  return grammarParseStatementBody(r, a, b, limit, pos);
}

/* Isi pemilihan grammar untuk SATU statement pada rentang [a,b).
 * Dipanggil grammarParseStatement baik untuk statement utuh maupun
 * bagian sebelum `;`/`,` (split statement satu baris). */
int grammarParseStatementBody(Request *r, int a, int b, int limit, int *pos) {
  Token *t = r->tokens;

  /* case may be emitted as a keyword by the processor; keep the grammar
   * check value-based as a defensive path while keyword tables evolve. */
  /*if (!strcmp(t->data[a].value, "case")) {*/
  /*int id = grammarParseCase(r, a, b, limit, pos);*/
  /*if (id != GRAMMAR_NO_MATCH) return id;*/
  /*}*/

  if (t->data[a].type == KEYWORD) {
    int id;
    /* async is an expression grammar, but at statement level it must stay
     * a standalone Async node rather than falling through expression-statement
     * wrapping (which currently creates a Return node). */
    if ((id = grammarParseAsyncExpr(r, a, b)) != GRAMMAR_NO_MATCH) {
      *pos = b;
      return id;
    }
    if ((id = grammarParseReturnKeyword(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseControl(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParsePrint(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseCase(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseIf(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseLoop(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseEnum(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
    if ((id = grammarParseModule(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
    /* `const x: number = 1` — keyword const membuka assignment decl;
     * didelegasikan ke grammarParseAssignment (mengonsumsi keyword). */
    if (t->data[a].value && !strcmp(t->data[a].value, "const")) {
      if ((id = grammarParseAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
    }
  }

  int id;
  if ((id = grammarParseFunction(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
  /* Class SEBELUM struct: `Name: Type {}` (safetyType + block) adalah
   * class (design/new_class.txt), bukan struct. Struct hanya bentuk
   * `Name {}` polos. */
  if ((id = grammarParseClass(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseStruct(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
  /* @marker (`@created`) — setelah decl grammar; wraps statement berikutnya. */
  if ((id = grammarParseMarker(r, a, b, limit, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseAnnotation(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseUpdate(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseConditionalAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseAssignment(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;
  if ((id = grammarParseComment(r, a, b, pos)) != GRAMMAR_NO_MATCH) return id;

  return grammarParseExpressionStatement(r, a, b, pos);
}
