#include <rupa.h>

/* Build an Annotation node from a single identifier/literal-id token whose
 * type was already normalized into .safetyType by the lexer (see
 * processIdentifier() in the lexer: "name: Type" collapses to ONE token
 * with .safetyType set, both at statement level and inside a function's
 * parameter list). Shared by grammarParseAnnotation() below and by the
 * parameter-list parsing in grammar_function.c, so both read the type the
 * same way instead of re-deriving it independently. */
int grammarAnnotationFromSafetyType(Request *r, int idx, int valueId) {
  Token *t = r->tokens;
  if (idx < 0 || idx >= t->length)
    return -1;
  if ((t->data[idx].type != IDENTIFIER && t->data[idx].type != LITERAL_ID) ||
      !t->data[idx].safetyType)
    return -1;

  /* This helper is used for declarations (annotations and parameters).
   * The name is therefore always a binding, never a value reference.
   * Do not feed it through parseAtom(), because a lexer token may be
   * LITERAL_ID when the same spelling occurs in a value context. */
  int name = createId(r->node, t->data[idx].value);
  int type = createId(r->node, t->data[idx].safetyType);
  return createAnnotation(r->node, name, type, valueId);
}

int grammarParseAnnotation(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;

  /* Fallback shape: a literal COLON token between identifier and type. In
   * practice the lexer normalizes simple "name: Type" patterns into a
   * single .safetyType-carrying token before this is ever reached (see the
   * branch below), so this mainly covers edge cases the normalizer doesn't
   * collapse (kept for fidelity/robustness with the original grammar). */
  if (t->data[a].type == IDENTIFIER && a + 1 < b &&
      t->data[a + 1].type == COLON) {
    int eq = -1;
    for (int i = a + 2; i < b; i++)
      if (t->data[i].type == ASSIGN) {
        eq = i;
        break;
      }
    int name = parseAtom(r, &t->data[a]);
    int type = grammarParseExpr(r, a + 2, eq >= 0 ? eq : b);
    int value = eq >= 0 ? grammarParseExpr(r, eq + 1, b) : -1;
    *pos = b;
    return createAnnotation(r->node, name, type, value);
  }

  /* Common case: bare "name: Type" with no trailing "= value" (a trailing
   * '=' belongs to the assignment grammar instead - grammar_assignment.c -
   * which also reads .safetyType to fill the Assignment node's Type
   * field). */
  if (t->data[a].type == IDENTIFIER && t->data[a].safetyType &&
      (a + 1 >= b || t->data[a + 1].type != ASSIGN)) {
    int id = grammarAnnotationFromSafetyType(r, a, -1);
    *pos = a + 1;
    return id;
  }

  return GRAMMAR_NO_MATCH;
}
