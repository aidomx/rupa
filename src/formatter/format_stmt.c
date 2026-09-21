#include <rupa.h>

void fmtAssign(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  if (n->assign.isConst) fmtStr(f, "const ");
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

static bool fmtKeepEmptyBlock(const Formatter *f) {
  if (!f || !f->config) return false;
  if (f->inClassMembers) return f->config->classMemberKeepEmptyBlock;
  return f->config->keepEmptyBlock;
}

/* ==================== Blank-line preservation (class/struct body) ==== */

/*
 * Baris sumber TERAKHIR yang dikonsumsi node statement (inklusif).
 *
 * Setiap AstNode membawa .line dari token pertamanya (createAst), jadi
 * baris awal statement selalu tersedia. Untuk baris akhir, statement
 * yang membentang beberapa baris harus dieksplisitkan: node container
 * mewarisi .line token PEMBUKA (mis. block '{' → NODE_BLOCK sendiri
 * ber-line '{'), bukan baris penutup '}' — sehingga endLine-nya harus
 * diambil dari statement anak TERAKHIR (rekursif ke body). Gap antar
 * member class/struct kemudian = next.line - endLine(prev); gap >= 2
 * berarti user menulis blank line dan formatter mempertahankannya.
 */
int fmtNodeEndLine(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return -1;
  AstNode *n = &node->ast[id];
  int end = n->line;
  switch (n->type) {
  case NODE_BLOCK:
    /* .row = line token penutup '}' (diisi grammarParseBlock). Untuk
     * block empty `{}\n}` ini satu-satunya info baris akhir — statement
     * anak tidak ada untuk mewarisi. */
    if (n->row > n->line)
      end = n->row;
    else if (n->block.length > 0 && n->block.statements)
      end = fmtNodeEndLine(node, n->block.statements[n->block.length - 1]);
    break;
  case NODE_FUNCTION_DECL:
    if (n->function.body >= 0)
      end = fmtNodeEndLine(node, n->function.body);
    break;
  case NODE_STRUCT_DECL:
    if (n->asStruct.body >= 0)
      end = fmtNodeEndLine(node, n->asStruct.body);
    break;
  case NODE_ENUM_DECL:
    if (n->asEnum.body >= 0)
      end = fmtNodeEndLine(node, n->asEnum.body);
    break;
  case NODE_CLASS_DECL:
    if (n->asClass.body >= 0)
      end = fmtNodeEndLine(node, n->asClass.body);
    break;
  case NODE_MARKER:
    /* @marker membungkus method decl berikutnya — endLine = method-nya. */
    if (n->asClass.body >= 0)
      end = fmtNodeEndLine(node, n->asClass.body);
    break;
  case NODE_IF: {
    int branch = n->asIf.elseBlock >= 0 ? n->asIf.elseBlock : n->asIf.thenBlock;
    if (branch >= 0)
      end = fmtNodeEndLine(node, branch);
    break;
  }
  case NODE_LOOP:
    if (n->loop.body >= 0)
      end = fmtNodeEndLine(node, n->loop.body);
    break;
  default:
    break;
  }
  return end > 0 ? end : -1;
}

/*
 * Baris sumber AWAL member. NODE_MARKER mewarisi .line token statement
 * yang di-wrap (method decl) — bukan baris '@'-nya. Baris '@' = baris
 * node NAMA marker (parseAtom pada token LITERAL_ID setelah '@'), jadi
 * unwrap dulu bila perlu.
 */
static int fmtMemberStartLine(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return -1;
  const AstNode *n = &node->ast[id];
  if (n->type == NODE_MARKER && n->asClass.name >= 0 && n->asClass.name < node->length)
    return node->ast[n->asClass.name].line;
  return n->line;
}

/*
 * Pemisah antar member dalam body class/struct (design/new_class.txt):
 * user yang menentukan ada tidaknya blank line — formatter hanya
 * menormalkan jumlahnya (maks SATU). Gap >= 2 baris sumber (di sini:
 * next.line - endLine(prev) >= 2, mis. '}' di baris 7 dan 'handler'
 * di baris 9) berarti ada blank line di sumber → dipertahankan;
 * `}\n@input` yang menempel tanpa blank line tetap rapat.
 */
void fmtMemberGap(Formatter *f, Node *node, int prevId, int nextId) {
  (void)node;
  (void)prevId;
  (void)nextId;
  fmtNewline(f);
}

void fmtBlock(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  if (n->block.length == 0 && !fmtKeepEmptyBlock(f)) {
    fmtChar(f, '{');
    fmtChar(f, '}');
    return;
  }
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
  } else if (thenNode && thenNode->type == NODE_BLOCK && thenNode->block.length == 0 &&
             !fmtKeepEmptyBlock(f)) {
    fmtStr(f, " {}");
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
    } else if (elseNode->type == NODE_BLOCK && elseNode->block.length == 0 &&
               !fmtKeepEmptyBlock(f)) {
      fmtStr(f, " {}");
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
  AstNode *body = (n->loop.body >= 0) ? &node->ast[n->loop.body] : NULL;
  if (body && body->type == NODE_BLOCK && body->block.length == 0 && !fmtKeepEmptyBlock(f)) {
    fmtStr(f, "{}");
    return;
  }
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
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
  AstNode *body = n->function.body >= 0 ? &node->ast[n->function.body] : NULL;
  if (body && body->type == NODE_BLOCK && body->block.length == 0 && !fmtKeepEmptyBlock(f)) {
    fmtStr(f, "{}");
    return;
  }
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->function.body >= 0) {
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
  AstNode *body = n->asStruct.body >= 0 ? &node->ast[n->asStruct.body] : NULL;
  if (body && body->type == NODE_BLOCK && body->block.length == 0 && !fmtKeepEmptyBlock(f)) {
    fmtStr(f, "{}");
    return;
  }
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->asStruct.body >= 0) {
    AstNode *body = &node->ast[n->asStruct.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        fmtNode(f, node, body->block.statements[i]);
        fmtMemberGap(f, node, body->block.statements[i],
                     i + 1 < body->block.length ? body->block.statements[i + 1] : -1);
      }
    } else {
      fmtNode(f, node, n->asStruct.body);
      fmtMemberGap(f, node, n->asStruct.body, -1);
    }
  }
  f->indent--;
  fmtStr(f, "}");
}

/* Enum decl (design/enum.txt): `enum Nama { ... }` — member berbentuk
 * Annotation(Name, Type, Value) / Identifier bare, cukup dibaca per node. */
void fmtEnumDecl(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtStr(f, "enum ");
  fmtNode(f, node, n->asEnum.name);
  fmtSep(f);
  AstNode *body = n->asEnum.body >= 0 ? &node->ast[n->asEnum.body] : NULL;
  if (body && body->type == NODE_BLOCK && body->block.length == 0 &&
      !fmtKeepEmptyBlock(f)) {
    fmtStr(f, "{}");
    return;
  }
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->asEnum.body >= 0) {
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        int m = body->block.statements[i];
        if (m < 0 || m >= node->length) continue;
        AstNode *mem = &node->ast[m];
        if (mem->type == NODE_ANNOTATION) {
          /* NAME[: Type][ = Value] */
          fmtNode(f, node, mem->annotation.name);
          if (mem->annotation.type >= 0) {
            fmtStr(f, ": ");
            fmtNode(f, node, mem->annotation.type);
          }
          if (mem->annotation.value >= 0) {
            fmtStr(f, " = ");
            fmtNode(f, node, mem->annotation.value);
          }
        } else {
          /* Identifier bare — auto-increment. */
          fmtNode(f, node, m);
        }
        fmtNewline(f);
      }
    } else {
      fmtNode(f, node, n->asEnum.body);
      fmtNewline(f);
    }
  }
  f->indent--;
  fmtStr(f, "}");
}

/* Class decl (design/new_class.txt): `Monster: MonsterType { ... }` atau
 * `main extends Monster { ... }` — anotasi `: Type` / extends
 * dipertahankan di output (penanda class). */
void fmtClassDecl(Formatter *f, Node *node, int id) {
  AstNode *n = &node->ast[id];
  fmtNode(f, node, n->asClass.name);
  if (n->asClass.type >= 0) {
    fmtStr(f, ": ");
    fmtNode(f, node, n->asClass.type);
  } else if (n->asClass.parent >= 0) {
    fmtStr(f, " extends ");
    fmtNode(f, node, n->asClass.parent);
  }
  fmtSep(f);
  AstNode *body = n->asClass.body >= 0 ? &node->ast[n->asClass.body] : NULL;
  if (body && body->type == NODE_BLOCK && body->block.length == 0 &&
      f->config && !f->config->classKeepEmptyBlock) {
    fmtStr(f, "{}");
    return;
  }
  fmtChar(f, '{');
  fmtNewline(f);
  f->indent++;
  if (n->asClass.body >= 0) {
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        bool oldClassMembers = f->inClassMembers;
        f->inClassMembers = true;
        fmtNode(f, node, body->block.statements[i]);
        f->inClassMembers = oldClassMembers;
        /* Baris kosong antar member mengikuti sumber (maks satu) —
         * `}\n@input` menempel tetap rapat, `}\n\n@input` dipertahankan. */
        fmtMemberGap(f, node, body->block.statements[i],
                     i + 1 < body->block.length ? body->block.statements[i + 1] : -1);
      }
    } else {
      fmtNode(f, node, n->asClass.body);
      fmtMemberGap(f, node, n->asClass.body, -1);
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
