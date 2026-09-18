#include <rupa.h>

void fmtAssign(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->assign.target);
  if (n->assign.type >= 0) {
    fmtStr(f, ": ");
    fmtNode(f, node, n->assign.type);
  }
  fmtStr(f, " = ");
  fmtNode(f, node, n->assign.value);
}

void fmtConditionalAssign(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->conditionalAssign.target);
  fmtStr(f, " ?= ");
  fmtNode(f, node, n->conditionalAssign.value);
}

void fmtBlock(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  for (int i = 0; i < n->block.length; i++) {
    fmtNode(f, node, n->block.statements[i]);
    fmtNewline(f);
  }
  f->indent--;
  fmtStr(f, "}");
}

void fmtIf(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "if ");
  fmtNode(f, node, n->asIf.condition);

  AstNode *thenNode = (n->asIf.thenBlock >= 0) ? &node->ast[n->asIf.thenBlock] : NULL;
  if (!n->asIf.isBlock) {
    fmtStr(f, ":");
    if (thenNode && thenNode->type == NODE_BLOCK && thenNode->block.length == 1) {
      fmtStr(f, " ");
      fmtNode(f, node, thenNode->block.statements[0]);
    } else if (n->asIf.thenBlock >= 0) {
      fmtStr(f, " ");
      fmtNode(f, node, n->asIf.thenBlock);
    }
  } else {
    fmtStr(f, " {");
    fmtNewline(f);
    f->indent++;
    if (thenNode && thenNode->type == NODE_BLOCK) {
      for (int i = 0; i < thenNode->block.length; i++) {
        fmtNode(f, node, thenNode->block.statements[i]);
        fmtNewline(f);
      }
    }
    f->indent--;
    fmtStr(f, "}");
  }

  if (n->asIf.elseBlock >= 0) {
    AstNode *elseNode = &node->ast[n->asIf.elseBlock];
    fmtStr(f, " else");
    if (elseNode->type == NODE_IF) {
      fmtStr(f, " ");
      fmtIf(f, node, n->asIf.elseBlock);
    } else {
      fmtStr(f, " {");
      fmtNewline(f);
      f->indent++;
      if (elseNode->type == NODE_BLOCK) {
        for (int i = 0; i < elseNode->block.length; i++) {
          fmtNode(f, node, elseNode->block.statements[i]);
          fmtNewline(f);
        }
      } else {
        fmtNode(f, node, n->asIf.elseBlock);
        fmtNewline(f);
      }
      f->indent--;
      fmtStr(f, "}");
    }
  }
}
void fmtLoop(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, n->loop.kind);
  fmtSep(f);
  fmtNode(f, node, n->loop.condition);
  fmtSep(f);
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  AstNode *body = (n->loop.body >= 0) ? &node->ast[n->loop.body] : NULL;
  if (body && body->type == NODE_BLOCK) {
    for (int i = 0; i < body->block.length; i++) {
      fmtNode(f, node, body->block.statements[i]);
      fmtNewline(f);
    }
  } else if (n->loop.body >= 0) {
    fmtNode(f, node, n->loop.body);
    fmtNewline(f);
  }
  f->indent--;
  fmtStr(f, "}");
}

void fmtFunctionDecl(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->function.name);
  fmtChar(f, '(');
  for (int i = 0; i < n->function.paramLength; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->function.params[i]);
  }
  fmtChar(f, ')');
  /* Return-type annotation: foo(): void { } */
  if (n->function.returnType >= 0) {
    char typeName[256];
    if (formatAstTypeName(node, n->function.returnType, typeName, sizeof(typeName))) {
      fmtStr(f, ": ");
      fmtStr(f, typeName);
    }
  }
  fmtSep(f);
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->function.body >= 0) {
    AstNode *body = &node->ast[n->function.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        fmtNode(f, node, body->block.statements[i]);
        fmtNewline(f);
      }
    } else {
      fmtNode(f, node, n->function.body);
      fmtNewline(f);
    }
  }
  f->indent--;
  fmtStr(f, "}");
}

void fmtStructDecl(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->asStruct.name);
  fmtSep(f);
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->asStruct.body >= 0) {
    AstNode *body = &node->ast[n->asStruct.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        fmtNode(f, node, body->block.statements[i]);
        fmtNewline(f);
      }
    } else {
      fmtNode(f, node, n->asStruct.body);
      fmtNewline(f);
    }
  }
  f->indent--;
  fmtStr(f, "}");
}

/* Class decl (design/new_class.txt): `Monster: MonsterType { ... }` —
 * anotasi `: Type` dipertahankan di output (penanda class). */
void fmtClassDecl(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->asClass.name);
  if (n->asClass.type >= 0) {
    fmtStr(f, ": ");
    fmtNode(f, node, n->asClass.type);
  }
  fmtSep(f);
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->asClass.body >= 0) {
    AstNode *body = &node->ast[n->asClass.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        fmtNode(f, node, body->block.statements[i]);
        fmtNewline(f);
      }
    } else {
      fmtNode(f, node, n->asClass.body);
      fmtNewline(f);
    }
  }
  f->indent--;
  fmtStr(f, "}");
}

/* Entry path: "a.create" disimpan flat di name + childrens rantai. */
static void fmtModEntryPath(Formatter *f, AstModEntry *e) {
  if (!e) return;
  fmtStr(f, e->name ? e->name : "?");
  if (e->type == MOD_WILD) fmtStr(f, ".*");
  for (AstModEntry *c = e->childrens; c; c = c->childrens) {
    fmtChar(f, '.');
    fmtStr(f, c->name ? c->name : "?");
  }
}

/*
 * NODE_MOD — 1 container untuk import & export.
 *
 * import a.create, b.login as auth, d.* from ../modules as m
 * import X, Y from rupa.os
 * import X
 * export x
 * export a, b from ./c
 * export c from ./c -> { a: private }
 */
void fmtMod(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  struct AstMod *mod = &n->mod;

  /* NamespaceDecl — namespace name { ... }: cetak header lalu
   * format statement di dalam body (NODE_BLOCK id). */
  if (mod->type == NamespaceDecl) {
    fmtStr(f, "namespace ");
    fmtStr(f, mod->source ? mod->source : "?");
    fmtStr(f, " {");
    if (mod->body >= 0 && mod->body < node->length) {
      AstNode *block = &node->ast[mod->body];
      if (block->type == NODE_BLOCK) {
        fmtNewline(f);
        f->indent++;
        for (int i = 0; i < block->block.length; i++) {
          fmtNode(f, node, block->block.statements[i]);
          fmtNewline(f);
        }
        f->indent--;
      }
    }
    fmtChar(f, '}');
    return;
  }

  if (mod->type == ExportDecl) {
    fmtStr(f, "export ");
  } else {
    fmtStr(f, "import ");
  }

  for (int i = 0; i < mod->entryCount; i++) {
    if (i > 0) fmtStr(f, ", ");
    AstModEntry *e = &mod->entries[i];
    fmtModEntryPath(f, e);
    if (e->key) {
      fmtStr(f, " as ");
      fmtStr(f, e->key);
    }
  }

  if (mod->source) {
    fmtStr(f, " from ");
    fmtStr(f, mod->source);
    if (mod->sourceAlias) {
      fmtStr(f, " as ");
      fmtStr(f, mod->sourceAlias);
    }
  }

  if (mod->policyCount > 0 && mod->policies) {
    fmtStr(f, " -> { ");
    for (int i = 0; i < mod->policyCount; i++) {
      if (i > 0) fmtStr(f, ", ");
      fmtStr(f, mod->policies[i].name ? mod->policies[i].name : "?");
      if (mod->policies[i].value) {
        fmtStr(f, ": ");
        fmtStr(f, mod->policies[i].value);
      }
    }
    fmtStr(f, " }");
  }
}

void fmtAsync(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "async ");
  fmtNode(f, node, n->async.request);
  int loader = n->async.handler >= 0 ? n->async.handler : n->async.loaderId;
  int timeout = n->async.timeout >= 0 ? n->async.timeout : n->async.timeoutId;

  if (!loader && !timeout) return;

  fmtStr(f, " -> ");
  fmtChar(f, '{');
  fmtStr(f, " ");
  fmtNode(f, node, loader);
  if (!timeout) {
    fmtChar(f, '}');
    return;
  }
  fmtStr(f, ", ");
  fmtNode(f, node, timeout);
  fmtStr(f, " ");
  fmtChar(f, '}');
}

void fmtCase(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "case ");
  fmtNode(f, node, n->asCase.subject);
  fmtStr(f, " {");
  fmtNewline(f);
  f->indent++;
  for (int i = 0; i < n->asCase.length; i++) {
    if (n->asCase.entries[i].wildcard) {
      fmtStr(f, "default:");
    } else {
      fmtStr(f, "case ");
      fmtNode(f, node, n->asCase.entries[i].pattern);
      fmtStr(f, ":");
    }
    fmtNewline(f);
    f->indent++;
    fmtNode(f, node, n->asCase.entries[i].body);
    fmtNewline(f);
    f->indent--;
  }
  f->indent--;
  fmtStr(f, "}");
}
