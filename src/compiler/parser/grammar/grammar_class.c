#include <rupa.h>

/* Class grammar (design/new_class.txt): class TANPA keyword. Bentuk
 * `Name: Type { ... }` — penanda class adalah `: Type` (TypeNode yang
 * harus sudah dideklarasikan, mis. MonsterType) — dibedakan dari
 * annotation deklarasi variable oleh block body `{ ... }`, bukan
 * `= value`.
 *
 * Bentuk ini sampai grammar sebagai SATU token IDENTIFIER/LITERAL_ID
 * dengan .safetyType (lexer normalizer: "Name: Type" collapse; lihat
 * createTokenId di lexer/factory.c) diikuti LBRACE. grammarParseStruct
 * (IDENTIFIER + LBRACE langsung) tidak akan cocok karena token pertama
 * bukan identifier polos; unit ini intercept SEBELUM grammarParseStruct
 * di grammar.c. */
int grammarParseClass(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;

  if (t->data[a].type != IDENTIFIER && t->data[a].type != LITERAL_ID)
    return GRAMMAR_NO_MATCH;
  if (!t->data[a].safetyType || !*t->data[a].safetyType)
    return GRAMMAR_NO_MATCH;

  /* Body wajib block `{ ... }` — tanpa itu ini annotation variable
   * biasa (`x: number = 1`), biarkan grammar lain menangani. */
  if (a + 1 >= limit || t->data[a + 1].type != LBRACE)
    return GRAMMAR_NO_MATCH;

  int close = grammarMatchClose(t, a + 1, limit, LBRACE, RBRACE);
  if (close < 0)
    return GRAMMAR_NO_MATCH;

  int name = parseAtom(r, &t->data[a]);
  int type = createTypeNode(r->node, t->data[a].safetyType);
  int body = grammarParseBlock(r, a + 1, close);
  *pos = close + 1;
  return createClassDecl(r->node, name, type, body);
}
