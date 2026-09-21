#include <rupa.h>

/*
 * Enum grammar (design/enum.txt):
 *
 *   enum TokenType {
 *     IDENTIFIER: string = "id"
 *     ASSIGN = 0
 *   }
 *
 * Token: KEYWORD(enum) IDENTIFIER/LITERAL_ID LBRACE ... RBRACE.
 * Member dibangun EKSPLISIT di sini (bukan statement acak dari
 * grammarParseBlock):
 *
 *   NAME = value          -> Annotation(Name, -1, Value)
 *   NAME: Type = value    -> Annotation(Name, Type, Value)
 *   NAME: Type            -> Annotation(Name, Type, -1)
 *   NAME                  -> Identifier (bare, tanpa nilai -> auto-increment)
 *
 * Interpretasi member (nilai default auto-increment) dilakukan saat
 * runtime, bukan parsing.
 */

/* Apakah token di posisi p membuka member baru? Member baru dimulai di
 * IDENTIFIER/LITERAL_ID yang diikuti COLON (pembawa tipe) atau ASSIGN
 * (pembawa nilai). Member bare (RED GREEN) TIDAK punya penanda — itu
 * ditangani lewat lebar member sebelumnya (bare = 1 token). */
static bool enumMemberHeader(Token *t, int p, int e) {
  if (p >= e)
    return false;
  if (t->data[p].type != IDENTIFIER && t->data[p].type != LITERAL_ID)
    return false;
  if (p + 1 >= e)
    return false;
  return t->data[p + 1].type == COLON || t->data[p + 1].type == ASSIGN;
}

/* Batas akhir (eksklusif) rentang token expression nilai member yang
 * mulai di `start`. Berhenti di separator, atau di token yang membuka
 * member berikutnya. Token pertama selalu diikutkan — `X = Y` tetap
 * terbaca meski Y disusul langsung '}'. */
static int enumValueEnd(Token *t, int start, int e) {
  int p = start;
  while (p < e) {
    TokenType type = t->data[p].type;
    if (type == NEWLINE || type == COMMA || type == SEMICOLON)
      break;
    if (p > start && enumMemberHeader(t, p, e))
      break;
    p++;
  }
  return p;
}

/* Bangun satu member enum dari token di posisi `s` (batas atas `e` =
 * posisi token penutup RBRACE). Return node id, atau -1 jika bukan
 * member valid. `consumed` menerima jumlah token yang dipakai. */
static int enumMember(Request *r, int s, int e, int *consumed) {
  Token *t = r->tokens;
  *consumed = 0;

  if (s >= e)
    return -1;
  DataToken *first = &t->data[s];
  if (first->type != IDENTIFIER && first->type != LITERAL_ID)
    return -1;

  int name = createId(r->node, first->value);
  if (name < 0)
    return -1;

  /* Bentuk A: lexer sudah menormalkan "NAME: Type" menjadi SATU token
   * dengan .safetyType (processIdentifier). Nilai menyusul bila ada
   * ASSIGN. */
  if (first->safetyType) {
    int type = createTypeNode(r->node, first->safetyType);
    if (type < 0)
      return -1;
    if (s + 1 < e && t->data[s + 1].type == ASSIGN && s + 2 < e) {
      int vend = enumValueEnd(t, s + 2, e);
      int value = grammarParseExpr(r, s + 2, vend);
      if (value < 0)
        return -1;
      *consumed = vend - s;
      return createAnnotation(r->node, name, type, value);
    }
    *consumed = 1;
    return createAnnotation(r->node, name, type, -1);
  }

  /* Bentuk B: colon belum dinormalkan — "NAME: Type [= value]". */
  if (s + 1 < e && t->data[s + 1].type == COLON) {
    int typeEnd = enumValueEnd(t, s + 2, e);
    int type = grammarParseExpr(r, s + 2, typeEnd);
    if (type < 0)
      return -1;
    if (typeEnd < e && t->data[typeEnd].type == ASSIGN &&
        typeEnd + 1 < e) {
      int vend = enumValueEnd(t, typeEnd + 1, e);
      int value = grammarParseExpr(r, typeEnd + 1, vend);
      if (value < 0)
        return -1;
      *consumed = vend - s;
      return createAnnotation(r->node, name, type, value);
    }
    *consumed = typeEnd - s;
    return createAnnotation(r->node, name, type, -1);
  }

  /* Bentuk C: "NAME = value" — Annotation tanpa tipe. */
  if (s + 1 < e && t->data[s + 1].type == ASSIGN && s + 2 < e) {
    int vend = enumValueEnd(t, s + 2, e);
    int value = grammarParseExpr(r, s + 2, vend);
    if (value < 0)
      return -1;
    *consumed = vend - s;
    return createAnnotation(r->node, name, -1, value);
  }

  /* Bentuk D: "NAME" polos — tanpa tipe, tanpa nilai (auto-increment).
   * Member disimpan sebagai Identifier bare (bukan statement acak), dan
   * tanpa Annotation wrapper supaya printer membacanya langsung. */
  *consumed = 1;
  return name;
}

int grammarParseEnum(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;

  if (t->data[a].type != KEYWORD || !t->data[a].value ||
      strcmp(t->data[a].value, "enum") != 0)
    return GRAMMAR_NO_MATCH;

  /* enum Nama { ... } */
  if (a + 2 >= limit ||
      (t->data[a + 1].type != IDENTIFIER && t->data[a + 1].type != LITERAL_ID) ||
      t->data[a + 2].type != LBRACE)
    return GRAMMAR_NO_MATCH;

  int close = grammarMatchClose(t, a + 2, limit, LBRACE, RBRACE);
  if (close < 0)
    return GRAMMAR_NO_MATCH;

  int name = parseAtom(r, &t->data[a + 1]);

  /* Parse body secara manual — per member. Member dipisah NEWLINE,
   * COMMA, SEMICOLON, atau berdiri berurutan satu baris (RED GREEN):
   * member bare hanya 1 token sehingga member berikutnya mulai tepat
   * setelahnya. */
  int items[256];
  int length = 0;
  int s = a + 3;
  while (s < close) {
    TokenType type = t->data[s].type;
    if (type == NEWLINE || type == COMMA || type == SEMICOLON) {
      s++;
      continue;
    }

    int consumed = 0;
    int member = enumMember(r, s, close, &consumed);
    if (member < 0 || consumed <= 0)
      return GRAMMAR_NO_MATCH;
    if (length < (int)(sizeof(items) / sizeof(items[0])))
      items[length++] = member;
    s += consumed;
  }

  *pos = close + 1;
  int body = createBlock(r->node, items, length);
  return createEnumDecl(r->node, name, body);
}
