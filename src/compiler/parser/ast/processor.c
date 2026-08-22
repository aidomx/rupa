#include <rupa.h>

/*
 * File ini sengaja dijaga tipis. Seluruh aturan grammar (bagaimana urutan
 * token diterjemahkan menjadi statement AST: keyword, assignment, deklarasi,
 * dst) hidup di src/compiler/parser/grammar/grammar.c. Di sini hanya berisi
 * API publik parser yang mengelola loop pembacaan token dan mendelegasikan
 * setiap statement ke grammarParseStatement().
 */

Response processAssignment(Request *req, Token *t, int init) {
  Response z = {-1, -1, -1, -1};
  int p = init, id = grammarParseStatement(req, &p, t->length);
  z.nodeId = id;
  return z;
}

Response processStandaloneExpression(Request *req, Token *t, int init) {
  return processAssignment(req, t, init);
}

Response generateHandler(Request *req, Token *t, int init) {
  return processAssignment(req, t, init);
}

Node *processGenerate(Request *req) {
  if (!req || !req->tokens)
    return NULL;
  int p = 0;
  while (p < req->tokens->length && !isToken(req->tokens, p, ENDOF)) {
    while (p < req->tokens->length && grammarIsWhitespace(req->tokens, p))
      p++;
    if (p >= req->tokens->length || isToken(req->tokens, p, ENDOF))
      break;
    int before = p, id = grammarParseStatement(req, &p, req->tokens->length);
    if (id >= 0)
      addToProgram(req->node, req->programId, id);
    if (p <= before)
      p++;
  }
  return req->node;
}
