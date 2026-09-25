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
    if (id >= 0) {
      addToProgram(req->node, req->programId, id);
    } else if (req->error) {
      DataToken *bad = &req->tokens->data[before];
      char message[256];
      snprintf(message, sizeof(message), "unexpected token '%s'",
               bad->value ? bad->value : "");
      addSourceError(req->error, "SyntaxError", message, bad->line, bad->row,
                     ERR_UNEXPECTED_TOKEN);
    }
    if (p <= before)
      p++;
  }

  syntaxMemoDetach(req); /* memo milik pool generasi ini — ikut habis */
  return req->node;
}
