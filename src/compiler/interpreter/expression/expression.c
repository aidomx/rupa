#include <rupa.h>

InterpreterResult interpretLiteral(Node *, AstNode *);
InterpreterResult interpretIdentifier(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretBinary(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretArray(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretObject(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretMember(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretAsync(Node *, int, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretAwait(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretStringInterp(Node *, AstNode *, RuntimeEnv *, Error *);

/* Fallback chain (`|`) dan then (`->`) memperlakukan "kandidat tidak
 * tersedia" sebagai alur normal, bukan error sungguhan - lihat
 * docs/grammar/assignment.md. Sebuah ReferenceError dari identifier yang
 * belum didefinisikan pada kandidat yang berakhir tidak dipakai (falsy)
 * persis kondisi itu, jadi dibuang di sini alih-alih ikut tercatat ke
 * pengguna. Jenis error LAIN (mis. TypeError dari ekspresi yang memang
 * rusak) tetap dipertahankan, karena itu bug sungguhan terlepas dari
 * kandidat mana yang akhirnya "menang". */
static void discardUndefinedErrors(Error *error, int mark) {
  if (!error) return;
  int keep = mark;
  for (int i = mark; i < error->size; i++) {
    if (error->info[i].type != ERR_UNDEFINED_VAR)
      error->info[keep++] = error->info[i];
  }
  error->size = keep;
}

/* Kumpulkan segmen rantai pipe nested-kiri (parseBinary) menjadi
 * urutan source (kiri→kanan). Return jumlah segmen; segs diisi id AST.
 * segs[0] = operand pertama rantai, dst. */
static int pipeSegments(Node *node, int id, int *segs, int cap) {
  int nseg = 0;
  int cur = id;
  while (cur >= 0 && cur < node->length &&
         node->ast[cur].type == NODE_FALLBACK && nseg < cap - 1) {
    segs[nseg++] = node->ast[cur].fallback.fallback;
    cur = node->ast[cur].fallback.primary;
  }
  if (cur >= 0 && cur < node->length && nseg < cap)
    segs[nseg++] = cur;
  for (int lo = 0; lo < nseg / 2; lo++) {
    int tmp = segs[lo];
    segs[lo] = segs[nseg - 1 - lo];
    segs[nseg - 1 - lo] = tmp;
  }
  return nseg;
}

InterpreterResult interpretExpression(Node *node, int id, RuntimeEnv *env,
                                      Error *error) {
  if (!node || id < 0 || id >= node->length)
    return resultNormal(valueNull());
  AstNode *ast = &node->ast[id];
  switch (ast->type) {
  case NODE_NUMBER:
  case NODE_DECIMAL:
  case NODE_BOOLEAN:
  case NODE_STRING:
  case NODE_NULLABLE:
    return interpretLiteral(node, ast);
  case NODE_STRING_INTERP:
    return interpretStringInterp(node, ast, env, error);
  case NODE_IDENTIFIER:
  case NODE_LITERAL_ID:
    return interpretIdentifier(node, ast, env, error);
  case NODE_BINARY:
    return interpretBinary(node, ast, env, error);
  case NODE_ARRAY:
    return interpretArray(node, ast, env, error);
  case NODE_OBJECT:
    return interpretObject(node, ast, env, error);
  case NODE_MEMBER:
    return interpretMember(node, ast, env, error);
  case NODE_CALL:
    return interpretCall(node, ast, env, error);
  case NODE_UPDATE:
    return interpretUpdate(node, ast, env, error);
  case NODE_SUBSCRIPT:
    return interpretSubscript(node, ast, env, error);
  case NODE_ASYNC:
    return interpretAsync(node, id, ast, env, error);
  case NODE_AWAIT:
    return interpretAwait(node, ast, env, error);
  case NODE_FALLBACK: {
    /* Rantai pipe tersimpan nested kiri (parseBinary left-assoc).
     * Rantai >= 3 segmen = ternary pipa c1 | v1 | c2 | v2 | ... | else
     * (lazy, pasangan kondisi→nilai); 2 segmen = or-else biasa. */
    int segs[64];
    int nseg = pipeSegments(node, id, segs, 64);

    if (nseg < 3) {
      int primaryId = nseg > 0 ? segs[0] : -1;
      int fallbackId = nseg > 1 ? segs[1] : -1;
      int mark = error ? error->size : 0;
      InterpreterResult primary =
          interpretExpression(node, primaryId, env, error);
      if (valueTruthy(primary.value))
        return primary;
      discardUndefinedErrors(error, mark);

      if (fallbackId < 0)
        return primary;
      mark = error ? error->size : 0;
      InterpreterResult fb =
          interpretExpression(node, fallbackId, env, error);
      if (!valueTruthy(fb.value))
        discardUndefinedErrors(error, mark);
      return fb;
    }

    /* Cascade ternary: segmen berupa THEN (cond -> val) — lengan
     * dievaluasi berurutan, nilai truthy pertama menang; segmen
     * terakhir = else. c1 -> v1 | c2 -> v2 | else. Tidak ada cek
     * flow: identifier undefined = falsy + error dibuang (pola
     * or-else lama). */
    if (node->ast[segs[0]].type == NODE_THEN) {
      for (int i = 0; i + 1 < nseg; i++) {
        int mark = error ? error->size : 0;
        InterpreterResult arm =
            interpretExpression(node, segs[i], env, error);
        if (valueTruthy(arm.value))
          return arm;
        discardUndefinedErrors(error, mark);
      }
      return interpretExpression(node, segs[nseg - 1], env, error);
    }

    /* Ternary pipa: pasangan kondisi→nilai dari kiri; segmen terakhir
     * (rantai ganjil) = else. Sama seperti or-else lama, identifier
     * undefined di arm mana pun = falsy + error undefined dibuang
     * (kegagalan arm bukan error program). */
    int i = 0;
    for (; i + 1 < nseg; i += 2) {
      int mark = error ? error->size : 0;
      InterpreterResult cond =
          interpretExpression(node, segs[i], env, error);
      if (valueTruthy(cond.value)) {
        mark = error ? error->size : 0;
        InterpreterResult val =
            interpretExpression(node, segs[i + 1], env, error);
        if (val.flow != FLOW_ERROR && valueTruthy(val.value))
          return val;
        /* nilai arm gagal/undefined → lanjut ke pasangan berikutnya */
        discardUndefinedErrors(error, mark);
      } else {
        discardUndefinedErrors(error, mark);
      }
    }
    if (i < nseg) {
      int mark = error ? error->size : 0;
      InterpreterResult val =
          interpretExpression(node, segs[i], env, error);
      discardUndefinedErrors(error, mark);
      return val;
    }
    return resultNormal(valueNull());
  }
  case NODE_THEN: {
    /* Bentuk ternary Rupa: cond -> then | else (THEN di atas rantai
     * FALLBACK; `x = cond -> then | else` tanpa `?` di depan). Rantai
     * then|else|... dievaluasi lazy: kondisi truthy → then, selain itu
     * rantai fallback di kanan dievaluasi sebagai or-else (bila ternyata
     * bukan rantai pipe, langsung nilai itu — semantik THEN lama). */
    if (node->ast[ast->then.result].type == NODE_FALLBACK) {
      int segs[64];
      int nseg = pipeSegments(node, ast->then.result, segs, 64);
      int mark = error ? error->size : 0;
      InterpreterResult cond =
          interpretExpression(node, ast->then.condition, env, error);
      if (valueTruthy(cond.value)) {
        discardUndefinedErrors(error, mark);
        /* THEN arm dikembalikan apa adanya (falsy pun — bukan or-else). */
        if (nseg > 0)
          return interpretExpression(node, segs[0], env, error);
        return interpretExpression(node, ast->then.result, env, error);
      }
      discardUndefinedErrors(error, mark);

      /* ELSE arm: nilai pertama dikembalikan apa adanya; segmen
       * berikutnya (bila ada) lanjut sebagai or-else setelah falsy. */
      for (int i = 1; i < nseg; i++) {
        InterpreterResult fb =
            interpretExpression(node, segs[i], env, error);
        if (valueTruthy(fb.value) || i == nseg - 1)
          return fb;
        mark = error ? error->size : 0;
        discardUndefinedErrors(error, mark);
      }
      return resultNormal(valueNull());
    }

    /* THEN polos: cond -> result : result hanya dievaluasi saat cond
     * truthy; selain itu hasilnya null. */
    int mark = error ? error->size : 0;
    InterpreterResult condition =
        interpretExpression(node, ast->then.condition, env, error);
    if (!valueTruthy(condition.value)) {
      discardUndefinedErrors(error, mark);
      return resultNormal(valueNull());
    }
    return interpretExpression(node, ast->then.result, env, error);
  }
  default:
    return resultNormal(valueNull());
  }
}
