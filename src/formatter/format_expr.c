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
  /* new T(args) — alokasi type-driven (design/new_memory.txt): arg
   * pertama adalah NAMA TIPE, dicetak tanpa koma setelah callee:
   * `new Contract(4)`, bukan `new(Contract, 4)`. */
  AstNode *callee = &node->ast[n->call.callee];
  const char *cname = callee->type == NODE_IDENTIFIER   ? callee->identifier.name
                      : callee->type == NODE_LITERAL_ID ? callee->string.value
                                                        : NULL;
  if (cname && !strcmp(cname, "new") && n->call.length >= 1) {
    fmtStr(f, "new ");
    fmtNode(f, node, n->call.args[0]);
    fmtChar(f, '(');
    for (int i = 1; i < n->call.length; i++) {
      if (i > 1) fmtStr(f, ", ");
      fmtNode(f, node, n->call.args[i]);
    }
    fmtChar(f, ')');
    return;
  }
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

/* String interpolation: parts bergantian string literal & expression.
 * {name} selalu di-parse sebagai NODE_IDENTIFIER → cetak `{name}`;
 * expression lain (call, member, binary, …) → cetak `{{expr}}`. */
void fmtStringInterp(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtChar(f, '"');
  for (int i = 0; i < n->stringInterp.length; i++) {
    int pid = n->stringInterp.parts[i];
    if (pid < 0 || pid >= node->length) continue;
    AstNode *p = &node->ast[pid];
    if (p->type == NODE_STRING) {
      const char *v = p->string.value;
      if (v) {
        size_t len = strlen(v);
        if (len >= 2 && v[0] == '"' && v[len - 1] == '"') {
          /* buang kutung pembuka/penutup, sisakan teks mentah */
          for (size_t k = 1; k + 1 < len; k++)
            fmtChar(f, v[k]);
        } else {
          fmtStr(f, v);
        }
      }
    } else if (p->type == NODE_IDENTIFIER) {
      fmtChar(f, '{');
      fmtNode(f, node, pid);
      fmtChar(f, '}');
    } else {
      fmtStr(f, "{{");
      fmtNode(f, node, pid);
      fmtStr(f, "}}");
    }
  }
  fmtChar(f, '"');
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

static void fmtObjectInline(Formatter *f, Node *node, AstNode *n) {
  bool trim = f->config && f->config->objectSpaceTrim;
  fmtChar(f, '{');

  if (!trim && n->object.length > 0) fmtChar(f, ' ');

  for (int i = 0; i < n->object.length; i++) {
    if (i > 0) {
      fmtChar(f, ',');
      if (!trim) fmtChar(f, ' ');
    }

    fmtNode(f, node, n->object.entries[i].key);

    if (n->object.entries[i].key != n->object.entries[i].value) {
      fmtChar(f, ':');
      if (!trim) fmtChar(f, ' ');

      fmtNode(f, node, n->object.entries[i].value);
    }
  }

  if (!trim && n->object.length > 0) fmtChar(f, ' ');

  fmtChar(f, '}');
}

static void fmtObjectMultiline(Formatter *f, Node *node, AstNode *n) {
  fmtChar(f, '{');
  if (n->object.length == 0) {
    fmtNewline(f);
    fmtStr(f, "}");
    return;
  }
  fmtNewline(f);
  f->indent++;
  for (int i = 0; i < n->object.length; i++) {
    fmtNode(f, node, n->object.entries[i].key);
    if (n->object.entries[i].key != n->object.entries[i].value) {
      fmtStr(f, ": ");
      fmtNode(f, node, n->object.entries[i].value);
    }
    if (i + 1 < n->object.length) fmtStr(f, ",");
    fmtNewline(f);
  }
  f->indent--;
  fmtStr(f, "}");
}

static bool fmtObjectWasMultiline(AstNode *n) {
  return n && n->row > n->line;
}

static size_t fmtObjectInlineLength(Formatter *f, Node *node, AstNode *n) {
  char *buf = NULL;
  size_t len = 0;
  FILE *mem = open_memstream(&buf, &len);
  if (!mem) return SIZE_MAX;

  Formatter tmp = *f;
  tmp.out = mem;
  tmp.needsIndent = false;
  tmp.lastWasNewline = false;
  tmp.pendingNewline = 0;
  fmtObjectInline(&tmp, node, n);
  fflush(mem);
  fclose(mem);
  free(buf);
  return len;
}

void fmtObject(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  const FormatterConfig *c = f->config;
  bool wasMultiline = fmtObjectWasMultiline(n);

  /* collapse=false means preserve the source layout of the object. */
  if (wasMultiline && c && !c->objectCollapse) {
    fmtObjectMultiline(f, node, n);
    return;
  }

  size_t inlineLength = fmtObjectInlineLength(f, node, n);
  int limit = c ? c->objectLimit : 100;
  if (inlineLength != SIZE_MAX && (limit <= 0 || (int)inlineLength <= limit)) {
    fmtObjectInline(f, node, n);
    return;
  }

  if (wasMultiline || (c && c->objectCollapse))
    fmtObjectMultiline(f, node, n);
  else
    fmtObjectInline(f, node, n);
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
  if (n->update.value >= 0) {
    /* Compound assignment: x += v */
    fmtNode(f, node, n->update.target);
    fmtStr(f, " ");
    fmtStr(f, n->update.op ? n->update.op : "?");
    fmtStr(f, " ");
    fmtNode(f, node, n->update.value);
    return;
  }
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
