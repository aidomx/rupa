#include <rupa.h>

void fmtIdentifier(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, n->identifier.name);
}

void fmtLiteralId(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, n->string.value);
}

void fmtNumber(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fprintf(f->out, "%d", n->number.value);
  f->needsIndent = false;
}

void fmtDecimal(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fprintf(f->out, "%s", n->decimal.lexeme);
  f->needsIndent = false;
}

void fmtBoolean(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, n->boolean.value ? "true" : "false");
}

void fmtString(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, n->string.value);
}

void fmtNull(Formatter *f) {
  fmtStr(f, "null");
}
