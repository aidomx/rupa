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
  fmtSep(f);

  AstNode *thenNode = (n->asIf.thenBlock >= 0) ? &node->ast[n->asIf.thenBlock] : NULL;
  if (thenNode && thenNode->type == NODE_BLOCK) {
    fmtChar(f, '{');
    fmtNewline(f);
    f->indent++;
    for (int i = 0; i < thenNode->block.length; i++) {
      fmtNode(f, node, thenNode->block.statements[i]);
      fmtNewline(f);
    }
    f->indent--;
    fmtStr(f, "}");
  } else {
    fmtNewline(f);
    f->indent++;
    if (n->asIf.thenBlock >= 0) fmtNode(f, node, n->asIf.thenBlock);
    fmtNewline(f);
    f->indent--;
    fmtStr(f, "}");
  }

  if (n->asIf.elseBlock >= 0) {
    fmtSep(f);
    AstNode *elseNode = &node->ast[n->asIf.elseBlock];
    if (elseNode->type == NODE_IF) {
      fmtStr(f, "else ");
      fmtIf(f, node, n->asIf.elseBlock);
    } else {
      fmtStr(f, "else");
      fmtSep(f);
      if (elseNode->type == NODE_BLOCK) {
        fmtChar(f, '{');
        fmtNewline(f);
        f->indent++;
        for (int i = 0; i < elseNode->block.length; i++) {
          fmtNode(f, node, elseNode->block.statements[i]);
          fmtNewline(f);
        }
        f->indent--;
        fmtStr(f, "}");
      } else {
        fmtNewline(f);
        f->indent++;
        fmtNode(f, node, n->asIf.elseBlock);
        fmtNewline(f);
        f->indent--;
        fmtStr(f, "}");
      }
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

void fmtModule(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "import ");

  if (n->module.value >= 0 && n->module.value < node->length &&
      node->ast[n->module.value].type == NODE_ARRAY) {
    AstNode *array = &node->ast[n->module.value];
    for (int i = 0; i < array->array.length; i++) {
      if (i > 0) fmtStr(f, ", ");
      fmtNode(f, node, array->array.elements[i]);
    }
  } else {
    fmtNode(f, node, n->module.value);
  }

  fmtStr(f, " from ");
  fmtNode(f, node, n->module.name);
}

void fmtModuleImport(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "import ");
  for (int i = 0; i < n->moduleImport.entryCount; i++) {
    if (i > 0) fmtStr(f, ", ");
    fmtNode(f, node, n->moduleImport.entries[i].pathNode);
    if (n->moduleImport.entries[i].isWildcard) fmtStr(f, ".*");
    if (n->moduleImport.entries[i].aliasNode >= 0) {
      fmtStr(f, " as ");
      fmtNode(f, node, n->moduleImport.entries[i].aliasNode);
    }
  }
  fmtStr(f, " from ");
  fmtNode(f, node, n->moduleImport.basePath);
  if (n->moduleImport.alias >= 0) {
    fmtStr(f, " as ");
    fmtNode(f, node, n->moduleImport.alias);
  }
}

void fmtExport(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "export ");

  if (n->astExport.namespaceName >= 0) fmtNode(f, node, n->astExport.namespaceName);
  fmtStr(f, " from ");
  fmtNode(f, node, n->astExport.sourcePath);
  if (n->astExport.policyCount > 0) {
    fmtStr(f, " -> { ");
    for (int i = 0; i < n->astExport.policyCount; i++) {
      if (i > 0) fmtStr(f, ", ");
      fmtNode(f, node, n->astExport.policies[i].nameNode);
      if (n->astExport.policies[i].policy) {
        fmtStr(f, ": ");
        fmtStr(f, n->astExport.policies[i].policy);
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
