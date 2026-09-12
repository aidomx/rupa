#include <rupa.h>

void fmtBinary(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->binary.left);
  fmtSep(f);
  fmtStr(f, n->binary.op);
  fmtSep(f);
  fmtNode(f, node, n->binary.right);
}

void fmtCall(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->call.callee);
  fmtChar(f, '(');
  for (int i = 0; i < n->call.length; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->call.args[i]);
  }
  fmtChar(f, ')');
}

void fmtPrint(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "print(");
  for (int i = 0; i < n->print.length; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->print.args[i]);
  }
  fmtChar(f, ')');
}

void fmtArray(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtChar(f, '[');
  for (int i = 0; i < n->array.length; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->array.elements[i]);
  }
  fmtChar(f, ']');
}

void fmtObject(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtChar(f, '{');
  for (int i = 0; i < n->object.length; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->object.entries[i].key);
    fmtStr(f, ": ");
    fmtNode(f, node, n->object.entries[i].value);
  }
  fmtChar(f, '}');
}

void fmtMember(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->member.object);
  fmtChar(f, '.');
  fmtNode(f, node, n->member.member);
}

void fmtSubscript(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->subscript.posId);
  fmtChar(f, '[');
  if (n->subscript.index >= 0) fmtNode(f, node, n->subscript.index);
  fmtChar(f, ']');
}

void fmtUpdate(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  if (n->update.prefix) fmtStr(f, n->update.op);
  fmtNode(f, node, n->update.target);
  if (!n->update.prefix) fmtStr(f, n->update.op);
}

void fmtAnnotation(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->annotation.name);
  if (n->annotation.type >= 0) {
    fmtStr(f, ": ");
    fmtNode(f, node, n->annotation.type);
  }
  if (n->annotation.value >= 0) {
    fmtStr(f, " = ");
    fmtNode(f, node, n->annotation.value);
  }
}

void fmtArrayType(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->arrayType.elementType);
  fmtStr(f, "[]");
}

void fmtReturn(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  if (n->asReturn.explicitReturn) fmtStr(f, "return");
  if (n->asReturn.expression >= 0) {
    if (n->asReturn.explicitReturn) fmtSep(f);
    fmtNode(f, node, n->asReturn.expression);
  }
}

void fmtThen(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->then.condition);
  fmtStr(f, " -> ");
  fmtNode(f, node, n->then.result);
}

void fmtFallback(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->fallback.primary);
  fmtStr(f, " | ");
  fmtNode(f, node, n->fallback.fallback);
}
