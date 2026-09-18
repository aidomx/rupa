#include <rupa.h>

/* Method marker grammar (design/new_class.txt poin 4/5): `@created` —
 * lexer mengemit AT + LITERAL_ID; unit ini menyusun NODE_MARKER yang
 * membungkus statement sesudahnya (method decl) sebagai value.
 *
 *   @created
 *   create(m: MonsterType) { ... }
 *
 * Marker TIDAK berdiri sendiri sebagai statement eksekusi: consumer
 * (classBuildInstance dsb.) membaca NODE_MARKER di body class dan
 * mengaitkan marker ke method yang di-wrap. */

int grammarParseMarker(Request *r, int a, int b, int limit, int *pos) {
  (void)b;
  Token *t = r->tokens;

  if (t->data[a].type != AT)
    return GRAMMAR_NO_MATCH;
  if (a + 1 >= limit || t->data[a + 1].type != LITERAL_ID)
    return GRAMMAR_NO_MATCH;

  int name = parseAtom(r, &t->data[a + 1]);

  /* Statement yang di-mark: baris berikutnya (method decl). */
  int p = a + 2;
  while (p < limit && grammarIsWhitespace(t, p))
    p++;
  int value = -1;
  if (p < limit) {
    int mpos = p;
    int m = grammarParseStatement(r, &mpos, limit);
    if (m != GRAMMAR_NO_MATCH && m >= 0) {
      value = m;
      p = mpos; /* maju MELEWATI statement yang di-mark. */
    } else {
      p = a + 2;
    }
  }

  *pos = p;
  return createMarker(r->node, name, value);
}
