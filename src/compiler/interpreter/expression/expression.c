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
    int mark = error ? error->size : 0;
    InterpreterResult primary =
        interpretExpression(node, ast->fallback.primary, env, error);
    if (valueTruthy(primary.value))
      return primary;
    discardUndefinedErrors(error, mark);

    mark = error ? error->size : 0;
    InterpreterResult fb =
        interpretExpression(node, ast->fallback.fallback, env, error);
    if (!valueTruthy(fb.value))
      discardUndefinedErrors(error, mark);
    return fb;
  }
  case NODE_THEN: {
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
