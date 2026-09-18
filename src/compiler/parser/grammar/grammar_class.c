#include <rupa.h>

/* Class grammar (design/new_class.txt): class TANPA keyword. Bentuk
 * `Name: Type { ... }` — penanda class adalah `: Type` (TypeNode yang
 * harus sudah dideklarasikan, mis. MonsterType) — dibedakan dari
 * annotation deklarasi variable oleh block body `{ ... }`, bukan
 * `= value`.
 *
 * Internal instantiation (langkah 2b): `c = Counter({value: 1})` —
 * nama class dipakai sebagai call dengan args. Args diteruskan ke
 * construct(args) otomatis (sifat optional: tanpa args construct
 * tetap dipanggil saat deklarasi). Parser menghasilkan NODE_CALL
 * biasa; dispatch runtime yang membedakan (nama = class terdaftar).
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

  /* `main extends Monster {}` (design/new_class.txt): extends otomatis
   * menandai class — orientasi presentation, bukan derivation. Token:
   * IDENTIFIER KEYWORD(extends) IDENTIFIER LBRACE. Validitas parent
   * dicek saat runtime (classBuildInstance), bukan parsing. */
  if (a + 3 < limit && t->data[a + 1].type == KEYWORD && t->data[a + 1].value &&
      !strcmp(t->data[a + 1].value, "extends") &&
      (t->data[a + 2].type == IDENTIFIER || t->data[a + 2].type == LITERAL_ID) &&
      t->data[a + 3].type == LBRACE) {
    int close = grammarMatchClose(t, a + 3, limit, LBRACE, RBRACE);
    if (close < 0)
      return GRAMMAR_NO_MATCH;
    int name = parseAtom(r, &t->data[a]);
    int body = grammarParseBlock(r, a + 3, close);
    int parent = createTypeNode(r->node, t->data[a + 2].value);
    *pos = close + 1;
    return createClassDeclExt(r->node, name, -1, body, parent);
  }

  /* Varian collapse lexer (defensive): safetyType "<name> extends <parent>". */
  if (t->data[a].safetyType && strstr(t->data[a].safetyType, "extends ")) {
    if (a + 1 >= limit || t->data[a + 1].type != LBRACE)
      return GRAMMAR_NO_MATCH;
    int close = grammarMatchClose(t, a + 1, limit, LBRACE, RBRACE);
    if (close < 0)
      return GRAMMAR_NO_MATCH;

    /* safetyType = "<name> extends <parent>". */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", t->data[a].safetyType);
    char *sp = strstr(buf, " extends ");
    if (!sp)
      return GRAMMAR_NO_MATCH;
    *sp = 0;
    const char *cname = buf;
    const char *pname = sp + strlen(" extends ");
    /* Buang postfix array type jika ada (defensive). */
    char *brk = strchr(pname, '[');
    if (brk) *brk = 0;
    if (!*cname || !*pname)
      return GRAMMAR_NO_MATCH;

    /* Catatan: validitas parent TIDAK bisa dicek saat parsing —
     * registry class terisi saat runtime (interpretClass). Pencocokan
     * induk dilakukan di classBuildInstance (analyzerClassParent). */

    int name = parseAtom(r, &t->data[a]);
    int body = grammarParseBlock(r, a + 1, close);
    int parent = createTypeNode(r->node, pname);
    /* Class extends TANPA `: Type` — type = -1 (orientasi induk cukup). */
    *pos = close + 1;
    return createClassDeclExt(r->node, name, -1, body, parent);
  }

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
