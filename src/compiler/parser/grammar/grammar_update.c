#include <rupa.h>

/*
 * Grammar update untuk increment/decrement dan compound assignment.
 *
 * Postfix: identifier++ / identifier--  DAN  member/subscript++:
 *   d.length++   arr[i]--   obj.k++
 * Prefix : ++identifier / --identifier
 * Compound: identifier += expr, -=, *=, /=, %= — termasuk target
 *   member/subscript: d.length += 1, arr[i] *= 2
 *
 * Update sengaja menjadi statement sendiri, bukan binary expression,
 * karena interpreter nantinya perlu mengetahui efek samping dan urutan
 * evaluasi prefix/postfix. Compound menyimpan operand kanan pada
 * update.value (node id) dan dievaluasi sebagai x OP v oleh runtime.
 *
 * Target kompleks (member/subscript) di-DESUGAR ke bentuk yang sudah
 * ditangani lengkap oleh back-end (interpretMemberAssign / member_set
 * IR, termasuk handle Contract via memoryMemberSet):
 *   E.m++  ->  E.m = E.m + 1          (NODE_MEMBER_ASSIGN + NODE_BINARY)
 *   E[i]-- ->  E[i] = E[i] - 1
 *   E.m+=v ->  E.m = E.m + v
 * Target identifier polos tetap NODE_UPDATE asli (urutan evaluasi
 * prefix/postfix dipertahankan di interpreter/IR).
 *
 * Catatan span: pemanggil statement memberikan span HINGGA `;`
 * ([x, ++, ;]). Trailing `;` di-strip dulu agar op terlihat di ujung
 * span — tanpa ini `x++;` pun tidak pernah match (x++; -> 5, bukan 6).
 */

/* Token operator biner sintetis (factory meng-clone value). */
static DataToken syntheticOpToken(const char *op) {
  DataToken tok;
  memset(&tok, 0, sizeof(tok));
  tok.value = (char *)op;
  tok.type = op[0] == '+'   ? PLUS
             : op[0] == '-' ? MINUS
             : op[0] == '*' ? STAR
             : op[0] == '/' ? SLASH
                            : PERCENT;
  return tok;
}

int grammarParseUpdate(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  int target = -1;
  const char *op = NULL;
  bool prefix = false;
  int value = -1;

  /* Trailing ';' milik statement — bukan bagian ekspresi update. */
  int end = b;
  while (end > a && t->data[end - 1].type == SEMICOLON) end--;

  if (end - a >= 2 &&
      (t->data[end - 1].type == INCREMENT || t->data[end - 1].type == DECREMENT)) {
    /* Postfix: <target>++ / <target>-- (target = sisa span). */
    target = grammarParseExpr(r, a, end - 1);
    op = t->data[end - 1].type == INCREMENT ? "++" : "--";
  } else if (end - a >= 2 &&
             (t->data[a].type == INCREMENT || t->data[a].type == DECREMENT)) {
    /* Prefix: ++<target> / --<target> */
    target = grammarParseExpr(r, a + 1, end);
    op = t->data[a].type == INCREMENT ? "++" : "--";
    prefix = true;
  } else {
    /* Compound assignment: target += value (juga -= *= /= %=).
     * Operator dicari di seluruh span (target boleh kompleks:
     * d.length += 1, arr[i] *= 2). */
    int opPos = -1;
    for (int i = a + 1; i < end; i++) {
      switch (t->data[i].type) {
      case PLUS_ASSIGN:
      case MINUS_ASSIGN:
      case STAR_ASSIGN:
      case SLASH_ASSIGN:
      case PERCENT_ASSIGN:
        opPos = i;
        break;
      default:
        break;
      }
      if (opPos >= 0) break;
    }
    if (opPos < 0)
      return GRAMMAR_NO_MATCH;

    switch (t->data[opPos].type) {
    case PLUS_ASSIGN: op = "+="; break;
    case MINUS_ASSIGN: op = "-="; break;
    case STAR_ASSIGN: op = "*="; break;
    case SLASH_ASSIGN: op = "/="; break;
    case PERCENT_ASSIGN: op = "%="; break;
    default: return GRAMMAR_NO_MATCH;
    }

    target = grammarParseExpr(r, a, opPos);
    value = grammarParseExpr(r, opPos + 1, end);
    if (target < 0 || value < 0)
      return -1;

    /* Target kompleks (member/subscript): desugar — NODE_UPDATE
     * runtime hanya menangani binding identifier. */
    AstNode *cn = &r->node->ast[target];
    if (cn->type != NODE_IDENTIFIER && cn->type != NODE_LITERAL_ID) {
      if (cn->type != NODE_MEMBER && cn->type != NODE_SUBSCRIPT)
        return GRAMMAR_NO_MATCH;
      DataToken tok2 = syntheticOpToken(op[0] == '-'   ? "-"
                                        : op[0] == '*' ? "*"
                                        : op[0] == '/' ? "/"
                                        : op[0] == '%' ? "%"
                                                       : "+");
      int read2 = cn->type == NODE_MEMBER
                      ? createMember(r->node, cn->member.object, cn->member.member)
                      : createSubscript(r->node, cn->subscript.posId, cn->subscript.index);
      if (read2 < 0) return -1;
      int bin2 = createBinary(r->node, &tok2, read2, value);
      if (bin2 < 0) return -1;
      int assign2 = createMemberAssign(r->node, target, bin2);
      if (assign2 < 0) return -1;
      *pos = b;
      return assign2;
    }

    *pos = b;
    return createUpdate(r->node, target, op, false, value);
  }

  if (target < 0)
    return -1;

  AstNode *tn = &r->node->ast[target];
  if (tn->type == NODE_IDENTIFIER || tn->type == NODE_LITERAL_ID) {
    /* Identifier polos: NODE_UPDATE asli — prefix/postfix dipertahankan. */
    *pos = b;
    return createUpdate(r->node, target, op, prefix, -1);
  }

  /* Target kompleks (member/subscript): desugar ke assignment +
   * binary — back-end menangani write-back ke handle/object. */
  if (tn->type != NODE_MEMBER && tn->type != NODE_SUBSCRIPT)
    return GRAMMAR_NO_MATCH; /* bukan target update yang valid */

  char base = op[0]; /* '+' atau '-' */
  DataToken opToken = syntheticOpToken(base == '-' ? "-" : "+");

  /* Re-parse target: postfix = token op di end-1; prefix = di a. */
  int tStart = prefix ? a + 1 : a;
  int tEnd = prefix ? end : end - 1;
  int targetDup = grammarParseExpr(r, tStart, tEnd);
  if (targetDup < 0) return -1;

  AstNode *dn = &r->node->ast[targetDup];
  if (dn->type != NODE_MEMBER && dn->type != NODE_SUBSCRIPT) return -1;

  int one = createNumber(r->node, 1);
  if (one < 0) return -1;

  int read = dn->type == NODE_MEMBER
                 ? createMember(r->node, dn->member.object, dn->member.member)
                 : createSubscript(r->node, dn->subscript.posId, dn->subscript.index);
  int bin = createBinary(r->node, &opToken, read, one);
  if (bin < 0) return -1;

  int assign = createMemberAssign(r->node, targetDup, bin);
  if (assign < 0) return -1;

  *pos = b;
  return assign;
}
