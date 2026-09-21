#include <rupa.h>

/* ============================================================
 * rewrite.c — AST -> IR
 *
 * Menurunkan AST Rupa menjadi IRModule berisi IRFunction/IRBlock
 * dan IRInstruction. IR sengaja independen dari RuntimeValue dan
 * NodeType (lihat lib/types/compiler/ir/ir.h): instruksi hanya
 * memakai IRValue/IRType, sehingga modul ini bisa dipakai sebagai
 * dasar transpile atau codegen tanpa menyentuh interpreter.
 *
 * Struktur hasil:
 *   - NODE_PROGRAM            -> IRFunction "main" + blok "entry"
 *   - NODE_FUNCTION_DECL      -> IRFunction tersendiri di module
 *   - Statement lain          -> instruksi di blok current
 *   - NODE_IF/NODE_LOOP/NODE_CASE -> blok + IR_BRANCH/IR_JUMP
 *
 * Control flow tidak menaruh flow di runtime: semua kontrol
 * berbentuk blok & branch sehingga urutan eksekusi ditentukan
 * penuh oleh IR. Konvensi nilai antar-blok bersifat phi-less:
 * temp yang belum di-store terbaca null (sequential store).
 * ============================================================ */

/* ==================== IRBuilder ==================== */

typedef struct IRBuilder IRBuilder;
typedef struct ScopeMap ScopeMap;

struct IRBuilder {
  Node *astRef;   /* pool AST sumber */
  IRModule *module;
  IRFunction *function;
  IRBlock *block;
  /* Cache operand berdasarkan AST node id supaya subexpression yang
   * sama tidak diturunkan dua kali. */
  IRValue **ops;
  int opCap;
  int opLen;
  /* Nama variable -> IRValue slot (local). Lintas scope dengan marker. */
  ScopeMap *scopes;
};

struct ScopeMap {
  char *name;
  IRValue *value;
  char *type; /* type deklarasi (x: T = ...) — kontrak reassignment */
  ScopeMap *next;
};

static IRValue *buildNode(IRBuilder *b, int id);
static IRValue *buildStatementValue(IRBuilder *b, int id);

static void builderInit(IRBuilder *b, Node *ast, IRModule *module) {
  memset(b, 0, sizeof(*b));
  b->astRef = ast;
  b->module = module;
}

/* ---- scope map (linked list dengan marker batas scope) ---- */

static IRValue *scopeFind(IRBuilder *b, const char *name) {
  if (!name) return NULL;
  for (ScopeMap *s = b->scopes; s; s = s->next)
    if (s->name && strcmp(s->name, name) == 0) return s->value;
  return NULL;
}

/* Type deklarasi variable (x: T = ...) untuk kontrak reassignment. */
static const char *scopeTypeOf(IRBuilder *b, const char *name) {
  if (!name) return NULL;
  for (ScopeMap *s = b->scopes; s; s = s->next)
    if (s->name && strcmp(s->name, name) == 0) return s->type;
  return NULL;
}

static void scopeBind(IRBuilder *b, const char *name, IRValue *value) {
  if (!name || !value) return;

  /* Rebind di scope sama: pertahankan type deklarasi lama (kontrak
   * reassignment tetap berlaku); type baru diset via scopeSetType. */
  for (ScopeMap *s = b->scopes; s; s = s->next) {
    if (s->name && strcmp(s->name, name) == 0) {
      s->value = value;
      return;
    }
  }

  ScopeMap *s = gccalloc(1, sizeof(*s));
  if (!s) return;
  s->name = gcdup(name);
  s->value = value;
  s->next = b->scopes;
  b->scopes = s;
}

/* Simpan/perbarui type deklarasi variable di scope map. */
static void scopeSetType(IRBuilder *b, const char *name, const char *type) {
  if (!name || !type) return;
  for (ScopeMap *s = b->scopes; s; s = s->next) {
    if (s->name && strcmp(s->name, name) == 0) {
      s->type = gcdup(type);
      return;
    }
  }
}

static void scopePush(IRBuilder *b) {
  ScopeMap *marker = gccalloc(1, sizeof(*marker));
  if (!marker) return;
  marker->name = gcdup("");
  marker->value = (IRValue *)b; /* sentinel: penanda batas scope */
  marker->next = b->scopes;
  b->scopes = marker;
}

static void scopePop(IRBuilder *b) {
  while (b->scopes) {
    ScopeMap *top = b->scopes;
    int isMarker = top->name && top->name[0] == '\0' && (void *)top->value == (void *)b;
    b->scopes = top->next;
    if (isMarker) return;
  }
}

static IRValue *cacheOp(IRBuilder *b, int id, IRValue *value) {
  if (id < 0 || !b->astRef || id >= b->astRef->length || !value) return value;
  if (id >= b->opCap) {
    int cap = b->opCap ? b->opCap * 2 : 64;
    while (id >= cap) cap *= 2;
    IRValue **grown = gcrealloc(b->ops, sizeof(IRValue *) * cap);
    if (!grown) return value;
    for (int i = b->opLen; i < cap; i++) grown[i] = NULL;
    b->ops = grown;
    b->opCap = cap;
  }
  if (id >= b->opLen) b->opLen = id + 1;
  b->ops[id] = value;
  return value;
}

/* ==================== Helpers ==================== */

static const char *nodeName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) return a->identifier.name;
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  return NULL;
}

static IRType *numberType(void) {
  /* number 64-bit: sizeof = 8 (long long), konsisten analyzer. */
  return irTypeCreate(IR_TYPE_NUMBER, "number", sizeof(long long));
}
static IRType *boolType(void) {
  return irTypeCreate(IR_TYPE_BOOLEAN, "boolean", sizeof(int));
}
static IRType *nullType(void) {
  return irTypeCreate(IR_TYPE_NULL, "null", 0);
}

static IROpcode opcodeFor(const char *op) {
  if (!op) return IR_NOP;
  if (!strcmp(op, "+")) return IR_ADD;
  if (!strcmp(op, "-")) return IR_SUB;
  if (!strcmp(op, "*")) return IR_MUL;
  if (!strcmp(op, "/")) return IR_DIV;
  if (!strcmp(op, "%")) return IR_MOD;
  if (!strcmp(op, "==")) return IR_EQ;
  if (!strcmp(op, "!=")) return IR_NE;
  if (!strcmp(op, "<")) return IR_LT;
  if (!strcmp(op, "<=")) return IR_LE;
  if (!strcmp(op, ">")) return IR_GT;
  if (!strcmp(op, ">=")) return IR_GE;
  return IR_NOP;
}

static int irNumericType(IRValue *v) {
  if (!v || !v->type) return 0;
  return v->type->kind == IR_TYPE_NUMBER || v->type->kind == IR_TYPE_DECIMAL;
}

/* Const-fold `a OP b` saat kedua operand konstanta numerik. */
static IRValue *foldBinary(IROpcode op, IRValue *l, IRValue *r) {
  if (!l || !r) return NULL;
  if (l->kind != IR_VALUE_CONSTANT || r->kind != IR_VALUE_CONSTANT) return NULL;
  if (!irNumericType(l) || !irNumericType(r)) return NULL;

  int decimal = l->type->kind == IR_TYPE_DECIMAL || r->type->kind == IR_TYPE_DECIMAL;
  /* number 64-bit: fold number op number di jalur integer 64-bit
   * (bukan lewat double) agar tidak kehilangan presisi. */
  if (!decimal) {
    long long a = l->data.constant.as.number;
    long long b = r->data.constant.as.number;
    switch (op) {
    case IR_ADD: return irNumber(a + b, numberType());
    case IR_SUB: return irNumber(a - b, numberType());
    case IR_MUL: return irNumber(a * b, numberType());
    case IR_DIV: if (b == 0) return NULL; return irNumber(a / b, numberType());
    case IR_MOD: if (b == 0) return NULL; return irNumber(a % b, numberType());
    default: break; /* perbandingan tetap lewat jalur double di bawah */
    }
  }
  double a = l->type->kind == IR_TYPE_DECIMAL ? l->data.constant.as.decimal
                                              : (double)l->data.constant.as.number;
  double b = r->type->kind == IR_TYPE_DECIMAL ? r->data.constant.as.decimal
                                              : (double)r->data.constant.as.number;

  double out = 0;
  switch (op) {
  case IR_ADD: out = a + b; break;
  case IR_SUB: out = a - b; break;
  case IR_MUL: out = a * b; break;
  case IR_DIV: if (b == 0) return NULL; out = a / b; break;
  case IR_MOD: if (b == 0) return NULL; out = (double)((int64_t)a % (int64_t)b); break;
  case IR_EQ: return irBoolean(a == b, boolType());
  case IR_NE: return irBoolean(a != b, boolType());
  case IR_LT: return irBoolean(a < b, boolType());
  case IR_LE: return irBoolean(a <= b, boolType());
  case IR_GT: return irBoolean(a > b, boolType());
  case IR_GE: return irBoolean(a >= b, boolType());
  default: return NULL;
  }

  return decimal
             ? irDecimal(out, irTypeCreate(IR_TYPE_DECIMAL, "decimal", sizeof(double)))
             : irNumber((int64_t)out, numberType());
}

static IRValue *emitBinary(IRBuilder *b, IROpcode op, IRValue *l, IRValue *r) {
  IRValue *folded = foldBinary(op, l, r);
  if (folded) return folded;

  IRType *t = irNumericType(l) && irNumericType(r) ? l->type : nullType();
  IRValue *res = irTemp(t);
  irEmit(b->block, irBinary(op, res, l, r));
  return res;
}

static IRValue *newLocal(IRBuilder *b, const char *name) {
  IRValue *v = irLocal(nullType(), name);
  scopeBind(b, name, v);
  return v;
}

static IRBlock *newBlock(IRBuilder *b, const char *prefix) {
  static char name[64];
  int nextId = b->function->last_block ? (int)b->function->last_block->id + 1 : 0;
  snprintf(name, sizeof(name), "%s.%d", prefix, nextId);
  return irBlockCreate(b->function, name);
}

/* Slot dibaca langsung: mesin eksekusi men-resolve IR_VALUE_LOCAL
 * berdasarkan nama saat load, jadi LOAD eksplisit tidak wajib. */
static IRValue *slotValue(IRValue *slot) {
  return slot;
}

/* ==================== Loop context (break/continue) ==================== */

typedef struct LoopCtx {
  IRBuilder *b;
  IRBlock *breakTarget;
  IRBlock *continueTarget;
  struct LoopCtx *parent;
} LoopCtx;

static LoopCtx *g_loop = NULL;

static void buildBreak(void) {
  if (g_loop && g_loop->breakTarget) irEmit(g_loop->b->block, irJump(g_loop->breakTarget));
}

static void buildContinue(void) {
  if (g_loop && g_loop->continueTarget)
    irEmit(g_loop->b->block, irJump(g_loop->continueTarget));
}

/* Nama tipe annotation (NODE_ARRAY_TYPE aware) sebagai string. */
static const char *annotationTypeName(IRBuilder *b, int typeId);

/* ==================== Statements ==================== */

static void buildProgram(IRBuilder *b, Node *node, int root) {
  IRFunction *fn = irFunctionCreate(b->module, "main", NULL);
  b->function = fn;
  b->block = irBlockCreate(fn, "entry");

  for (AstDeclaration *d = node->ast[root].program.declarations; d; d = d->next)
    buildStatementValue(b, d->nodeId);

  irEmit(b->block, irReturn(irNull(NULL)));
}

static void buildFunctionDecl(IRBuilder *b, Node *node, const AstNode *a) {
  const char *name = nodeName(node, a->function.name);

  IRFunction *savedFn = b->function;
  IRBlock *savedBlock = b->block;
  ScopeMap *savedScope = b->scopes;

  /* Return-type annotation (`foo(): void { }`): catat di IRFunction.
   * void = IR_TYPE_VOID; tipe lain beri nama saja (belum di-enforce). */
  IRType *retT = NULL;
  if (a->function.returnType >= 0) {
    const char *rtName = annotationTypeName(b, a->function.returnType);
    if (rtName && !strcmp(rtName, "void"))
      retT = irTypeCreate(IR_TYPE_VOID, "void", 0);
    else if (rtName)
      retT = irTypeCreate(IR_TYPE_VOID, rtName, 0); /* placeholder name */
  }

  IRFunction *fn = irFunctionCreate(b->module, name ? name : "anonymous", retT);
  b->function = fn;
  b->block = irBlockCreate(fn, "entry");
  b->scopes = NULL;

  for (int i = 0; i < a->function.paramLength; i++) {
    const char *pname = nodeName(node, a->function.params[i]);
    if (!pname) continue;
    IRValue *p = irParam(nullType(), pname);
    scopeBind(b, pname, p);
    IRValue **grown = gcrealloc(fn->params, sizeof(IRValue *) * (fn->param_count + 1));
    if (grown) {
      grown[fn->param_count++] = p;
      fn->params = grown;
    }
  }

  buildStatementValue(b, a->function.body);

  /* Pastikan blok terakhir selalu berakhir dengan return. */
  if (b->block->last && b->block->last->op != IR_RETURN)
    irEmit(b->block, irReturn(irNull(NULL)));

  b->function = savedFn;
  b->block = savedBlock;
  b->scopes = savedScope;

  /* Bind fungsi ke env via IRFunction slot — dipakai execCall dan
   * trampoline interpretNode (callLoader/callHandler async, callback).
   * Slot IR_VALUE_FUNCTION menandakan "fungsi dengan nama ini". */
  if (name) {
    IRValue *slot = irFunctionValue(nullType(), name);
    irEmit(b->block, irStore(slot, irFunctionValue(nullType(), name)));
  }
}

/* Nama tipe annotation (NODE_ARRAY_TYPE aware) sebagai string. */
static const char *annotationTypeName(IRBuilder *b, int typeId) {
  static char buffer[256];
  if (!formatAstTypeName(b->astRef, typeId, buffer, sizeof(buffer))) return NULL;
  return buffer;
}

/* Node value adalah panggilan `new Contract(...)`? (C-blok design
 * new_memory.txt — alokasi type-driven dari anotasi.) */
static bool memoryContractIsContract(Node *node, int valueId) {
  if (!node || valueId < 0 || valueId >= node->length) return false;
  AstNode *call = &node->ast[valueId];
  if (call->type != NODE_CALL || call->call.length < 1) return false;
  AstNode *callee = &node->ast[call->call.callee];
  const char *fn = callee->type == NODE_IDENTIFIER   ? callee->identifier.name
                   : callee->type == NODE_LITERAL_ID ? callee->string.value
                                                     : NULL;
  if (!fn || strcmp(fn, "new") != 0) return false;
  AstNode *arg = &node->ast[call->call.args[0]];
  const char *tname = arg->type == NODE_IDENTIFIER   ? arg->identifier.name
                      : arg->type == NODE_LITERAL_ID ? arg->string.value
                                                     : NULL;
  return tname && !strcmp(tname, "Contract");
}

static void buildAssign(IRBuilder *b, Node *node, const AstNode *a, int id) {
  (void)node;
  const char *name = nodeName(b->astRef, a->assign.target);

  /* new Contract (C1–C4): intercept SEBELUM buildNode — alokasi dari
   * anotasi via IR_ALLOC(IR_TYPE_POINTER). Nama tipe elemen (C2: 'T[]'
   * direduksi ke 'T' saat lookup) di-resolve executor dari payload
   * alloc.type->name; count = arg kedua Contract (default 1).
   * C1 (tanpa anotasi) tak mungkin di sini — blok ini hanya jalan
   * bila assign.type >= 0; `p = new Contract()` polos jatuh ke jalur
   * new biasa dan ditolak oleh memoryNewCall (tipe Contract tak dikenal). */
  if (a->assign.type >= 0 && memoryContractIsContract(b->astRef, a->assign.value)) {
    const char *ann = annotationTypeName(b, a->assign.type);
    AstNode *call = &b->astRef->ast[a->assign.value];
    IRValue *count =
        call->call.length == 2 ? buildNode(b, call->call.args[1])
                               : irNumber(1, numberType());
    IRType *ptrT = irTypeCreate(IR_TYPE_POINTER, ann ? ann : "ptr", sizeof(void *));
    IRValue *res = irTemp(ptrT);
    irEmit(b->block, irAlloc(res, ptrT, count, 1));
    IRValue *slot = scopeFind(b, name);
    if (!slot) slot = newLocal(b, name);
    if (ann) scopeSetType(b, name, ann);
    irEmit(b->block, irStore(slot, res));
    return;
  }

  /* Write-through string slot (design/str_memory.txt): name = "rudi" /
   * name = dupl(...) pada handle Contract string — decision butuh env
   * & binding runtime, jadi trampoline seluruh NODE_ASSIGN. */
  {
    AstNode *targetAst = &b->astRef->ast[a->assign.target];
    const char *tname = targetAst->type == NODE_IDENTIFIER   ? targetAst->identifier.name
                        : targetAst->type == NODE_LITERAL_ID ? targetAst->string.value
                                                             : NULL;
    if (tname) {
      const char *declared = scopeTypeOf(b, tname);
      char probe[1];
      bool valueIsHandle = memoryNewTypeName(b->astRef, a->assign.value, probe, sizeof(probe));
      if (declared && !strcmp(declared, "string") && !valueIsHandle) {
        /* Write-through string slot: tulis value ke slot handle yang
         * sudah ada (op native IR_STRSLOT_SET — registry v3 memutuskan). */
        IRValue *slotPtr = scopeFind(b, tname);
        if (!slotPtr) slotPtr = newLocal(b, tname);
        IRValue *value = buildNode(b, a->assign.value);
        if (value) irEmit(b->block, irStrSlotSet(slotPtr, value));
        return;
      }
    }
  }

  IRValue *value = buildNode(b, a->assign.value);
  if (!name || !value) return;

  /* Const binding: flag di store (irStoreAt) — executor mengunci slot
   * setelah write pertama dan menolak store berikutnya (ConstError).
   * Bukan properti nilai, jadi tidak ada pseudo-type check "const". */

  /* Typed assign (x: T = v): semantic check sebelum store + catat type
   * deklarasi di scope map — kontrak reassignment selanjutnya. */
  if (a->assign.type >= 0) {
    const char *typeName = annotationTypeName(b, a->assign.type);
    if (typeName) {
      analyzerSetErrorLocation(b->astRef, a->assign.type);
      irEmit(b->block, irCheckAt(value, typeName, a->assign.value));
    }
  }

  IRValue *slot = scopeFind(b, name);
  if (!slot) slot = newLocal(b, name);

  /* Catat type SETELAH slot ada (scopeSetType butuh entry yang sudah
   * ter-bind), lalu reassignment polos (x = v) ke variable yang pernah
   * dideklarasikan divalidasi terhadap type itu (kontrak permanen). */
  if (a->assign.type >= 0) {
    const char *typeName = annotationTypeName(b, a->assign.type);
    if (typeName) scopeSetType(b, name, typeName);
  } else if (!a->assign.isConst) {
    /* new T() type-driven: catat type dari alokasi — kontrak permanen
     * untuk reassignment polos juga berlaku pada handle new/del. */
    char newType[256];
    if (memoryNewTypeName(b->astRef, a->assign.value, newType, sizeof(newType))) {
      scopeSetType(b, name, newType);
    } else {
      const char *declared = scopeTypeOf(b, name);
      if (declared)
        irEmit(b->block, irCheckAt(value, declared, a->assign.value));
    }
  }

  /* nodeId = statement assign (bukan node value) — lokasi ConstError
   * menunjuk baris assignment, bukan ekspresi sisi kanan. */
  irEmit(b->block, irStoreAt(slot, value, a->assign.isConst, id));
}

static void buildConditionalAssign(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  const char *name = nodeName(b->astRef, a->conditionalAssign.target);
  if (!name) return;

  IRValue *slot = scopeFind(b, name);
  if (!slot) slot = newLocal(b, name);

  IRBlock *evalBlock = newBlock(b, "cond.eval");
  IRBlock *endBlock = newBlock(b, "cond.end");

  /* Truthiness slot native: falsy (0/null/""/...) -> jalankan sisi kanan. */
  irEmit(b->block, irBranch(slot, endBlock, evalBlock));

  b->block = evalBlock;
  IRValue *value = buildNode(b, a->conditionalAssign.value);
  if (value) {
    /* Kontrak type permanen berlaku juga di x ?= v. */
    const char *declared = scopeTypeOf(b, name);
    if (declared)
      irEmit(b->block, irCheckAt(value, declared, a->conditionalAssign.value));
    irEmit(b->block, irStore(slot, value));
  }
  irEmit(b->block, irJump(endBlock));

  b->block = endBlock;
}

static void buildAnnotation(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  const char *name = nodeName(b->astRef, a->annotation.name);
  if (!name) return;

  IRValue *slot = scopeFind(b, name);
  if (!slot) slot = newLocal(b, name);
  if (a->annotation.value < 0) return;

  /* new Contract (C1–C4) — sejajar buildAssign. */
  if (memoryContractIsContract(b->astRef, a->annotation.value)) {
    const char *ann = annotationTypeName(b, a->annotation.type);
    AstNode *call = &b->astRef->ast[a->annotation.value];
    IRValue *count =
        call->call.length == 2 ? buildNode(b, call->call.args[1])
                               : irNumber(1, numberType());
    IRType *ptrT = irTypeCreate(IR_TYPE_POINTER, ann ? ann : "ptr", sizeof(void *));
    IRValue *res = irTemp(ptrT);
    irEmit(b->block, irAlloc(res, ptrT, count, 1));
    if (ann) scopeSetType(b, name, ann);
    irEmit(b->block, irStore(slot, res));
    return;
  }

  IRValue *value = buildNode(b, a->annotation.value);
  if (!value) return;

  /* c: T = v — semantic check sebelum store + catat type deklarasi. */
  const char *typeName = annotationTypeName(b, a->annotation.type);
  if (typeName) {
    analyzerSetErrorLocation(b->astRef, a->annotation.type);
    irEmit(b->block, irCheckAt(value, typeName, a->annotation.value));
    scopeSetType(b, name, typeName);
  }

  irEmit(b->block, irStore(slot, value));
}

static void buildPrint(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  for (int i = 0; i < a->print.length; i++) {
    IRValue *v = buildNode(b, a->print.args[i]);
    if (!v) continue;
    IRValue *callee = irFunctionValue(nullType(), "print");
    IRValue **args = gccalloc(1, sizeof(IRValue *));
    if (!args) continue;
    args[0] = v;
    irEmit(b->block, irCall(irTemp(nullType()), callee, args, 1));
  }
}

static void buildReturn(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  /* explicitReturn=false adalah expression statement (REPL semantic):
   * evaluasi untuk efek samping saja, JANGAN emit IR_RETURN — kalau
   * tidak, sisa program setelah statement ini jadi dead code. */
  if (!a->asReturn.explicitReturn) {
    if (a->asReturn.expression >= 0) buildNode(b, a->asReturn.expression);
    return;
  }
  IRValue *value = a->asReturn.expression >= 0 ? buildNode(b, a->asReturn.expression)
                                               : irNull(NULL);
  irEmit(b->block, irReturn(value ? value : irNull(NULL)));
}

static void buildBlock(IRBuilder *b, int id) {
  if (id < 0 || id >= b->astRef->length) return;
  AstNode *a = &b->astRef->ast[id];
  /* Branch `else if` memproduksi node IF langsung (bukan NODE_BLOCK) —
   * baca union block tanpa cek type = garbage length → segfault. */
  if (a->type != NODE_BLOCK) {
    buildStatementValue(b, id);
    return;
  }
  for (int i = 0; i < a->block.length; i++)
    buildStatementValue(b, a->block.statements[i]);
}

static void buildIf(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  IRValue *cond = buildNode(b, a->asIf.condition);
  if (!cond) return;

  IRBlock *thenBlock = newBlock(b, "then");
  IRBlock *elseBlock = a->asIf.elseBlock >= 0 ? newBlock(b, "else") : NULL;
  IRBlock *endBlock = newBlock(b, "endif");

  irEmit(b->block, irBranch(cond, thenBlock, elseBlock ? elseBlock : endBlock));

  b->block = thenBlock;
  buildBlock(b, a->asIf.thenBlock);
  irEmit(b->block, irJump(endBlock));

  if (elseBlock) {
    b->block = elseBlock;
    buildBlock(b, a->asIf.elseBlock);
    irEmit(b->block, irJump(endBlock));
  }

  b->block = endBlock;
}

static void buildLoop(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  bool isFor = a->loop.kind && !strcmp(a->loop.kind, "for");
  bool isRev = a->loop.kind && !strcmp(a->loop.kind, "rev");

  IRBlock *condBlock = newBlock(b, "loop.cond");
  IRBlock *bodyBlock = newBlock(b, "loop.body");
  IRBlock *endBlock = newBlock(b, "loop.end");
  IRBlock *stepBlock = newBlock(b, "loop.step");

  LoopCtx ctx = {.b = b,
                 .breakTarget = endBlock,
                 .continueTarget = (isFor || isRev) ? stepBlock : condBlock,
                 .parent = g_loop};
  g_loop = &ctx;

  if (isFor || isRev) {
    /* Semantik range-loop Rupa — disetarakan baris-per-baris dengan
     * interpretForLoop / interpretRevLoop:
     *   - counter INTERNAL (hidden) dikendalikan loop; variable user
     *     di-sinkronkan ke counter di awal TIAP iterasi, jadi modifikasi
     *     variable di body tidak memengaruhi jumlah iterasi.
     *   - for: counter mulai 0 (shorthand) atau dari nilai variable saat
     *     ini (eksplisit, dengan aturan current == -1 -> mulai dari bound).
     *   - rev: counter mulai dari nilai variable (shorthand & ident-left)
     *     atau dari literal kiri (ident-right, bound = 0).
     *   - Akhir: for -> variable = bound; rev -> variable = nilai pertama
     *     yang gagal kondisi (shorthand: -1). */
    const AstNode *condNode = a->loop.condition >= 0 ? &b->astRef->ast[a->loop.condition] : NULL;
    const char *name = NULL;
    IRValue *slot = NULL;
    IRValue *boundV = NULL;
    IRValue *startFrom = NULL; /* NULL => mulai dari konstanta 0 */
    IROpcode cmpOp = IR_NOP;
    bool neg1Select = false;   /* aturan current == -1 -> bound (for eksplisit ident-left) */

    bool fresh = false; /* variable belum pernah di-assign sebelum loop */
    bool isShorthand =
        condNode && (condNode->type == NODE_IDENTIFIER || condNode->type == NODE_LITERAL_ID);

    if (condNode && (condNode->type == NODE_IDENTIFIER || condNode->type == NODE_LITERAL_ID)) {
      /* Shorthand: `for i` / `rev i`. Bound = nilai variable SAAT INI
       * (snapshot), bukan slot — body menimpa slot tiap iterasi. */
      name = nodeName(b->astRef, a->loop.condition);
      slot = scopeFind(b, name);
      if (!slot && name) {
        slot = newLocal(b, name);
        fresh = true;
      }
      if (isFor) {
        cmpOp = IR_LT;
        if (fresh) {
          boundV = irNumber(0, numberType());
        } else {
          /* Snapshot bound: body menimpa slot tiap iterasi, jadi bound
           * tidak boleh dibaca dari slot saat perbandingan. */
          IRValue *snap = irTemp(numberType());
          irEmit(b->block, irStore(snap, slotValue(slot)));
          boundV = snap;
        }
      } else {
        cmpOp = IR_GE;
        boundV = irNumber(0, numberType());
        startFrom = fresh ? NULL : slotValue(slot);
      }
    } else if (condNode && condNode->type == NODE_BINARY) {
      const char *opStr = condNode->binary.op;
      name = nodeName(b->astRef, condNode->binary.left);
      if (name) {
        /* Ident di kiri: `for i < 10` / `rev i > 0`. */
        slot = scopeFind(b, name);
        if (!slot) {
          slot = newLocal(b, name);
          fresh = true;
        }
        boundV = buildNode(b, condNode->binary.right);
        startFrom = fresh ? NULL : slotValue(slot);
        neg1Select = isFor && !fresh;
      } else {
        /* Ident di kanan: `for 10 < i` / `rev 10 > i`. */
        name = nodeName(b->astRef, condNode->binary.right);
        slot = scopeFind(b, name);
        if (!slot && name) {
          slot = newLocal(b, name);
          fresh = true;
        }
        if (isRev) {
          boundV = irNumber(0, numberType());
          startFrom = buildNode(b, condNode->binary.left);
        } else {
          boundV = buildNode(b, condNode->binary.left);
          startFrom = fresh ? NULL : slotValue(slot);
        }
      }
      cmpOp = opcodeFor(opStr);
      if (cmpOp == IR_NOP) cmpOp = isFor ? IR_LT : IR_GE;
    }

    IRValue *hidden = irTemp(numberType());

    if (neg1Select && slot && boundV) {
      /* Interpreter: value = current == -1 ? bound : current (ekspresi
       * `for` setelah `rev` — i bernilai -1 — langsung habis). */
      IRBlock *negBlock = newBlock(b, "for.neg");
      IRBlock *normBlock = newBlock(b, "for.norm");
      IRBlock *mergeBlock = newBlock(b, "for.merge");
      IRValue *isNeg = irTemp(boolType());
      irEmit(b->block, irBinary(IR_EQ, isNeg, slot, irNumber(-1, numberType())));
      irEmit(b->block, irBranch(isNeg, negBlock, normBlock));

      b->block = negBlock;
      irEmit(b->block, irStore(hidden, boundV));
      irEmit(b->block, irJump(mergeBlock));

      b->block = normBlock;
      irEmit(b->block, irStore(hidden, slot));
      irEmit(b->block, irJump(mergeBlock));

      b->block = mergeBlock;
    } else {
      /* Start counter — sejajar semGet interpreter:
       *   for eksplisit fresh -> 0; rev ident-kiri fresh -> bound;
       *   shorthand fresh rev -> -1 (langsung habis, var tak tersentuh). */
      IRValue *start = startFrom;
      if (!start && fresh && isRev)
        start = isShorthand ? irNumber(-1, numberType()) : boundV;
      if (!start) start = irNumber(0, numberType());
      irEmit(b->block, irStore(hidden, start));
    }

    irEmit(b->block, irJump(condBlock));
    b->block = condBlock;

    IRValue *cmp = irTemp(boolType());
    irEmit(b->block, irBinary(cmpOp, cmp, hidden, boundV));
    irEmit(b->block, irBranch(cmp, bodyBlock, endBlock));

    b->block = bodyBlock;
    /* Sinkronkan variable user ke counter internal di awal iterasi. */
    if (slot) irEmit(b->block, irStore(slot, hidden));
    buildBlock(b, a->loop.body);
    irEmit(b->block, irJump(stepBlock));

    b->block = stepBlock;
    IRValue *one = irNumber(1, numberType());
    IRValue *next = irTemp(numberType());
    if (isFor) {
      irEmit(b->block, irBinary(IR_ADD, next, hidden, one));
    } else {
      IRValue *neg = irTemp(numberType());
      irEmit(b->block, irUnary(IR_NEG, neg, one));
      irEmit(b->block, irBinary(IR_ADD, next, hidden, neg));
    }
    irEmit(b->block, irStore(hidden, next));
    irEmit(b->block, irJump(condBlock));

    b->block = endBlock;
    /* Nilai akhir variable user: for -> bound; rev -> counter internal
     * (nilai pertama yang gagal kondisi; shorthand rev berakhir -1).
     * Shorthand fresh tidak pernah menyentuh variable (interpreter
     * semGet gagal -> range false -> loop skip tanpa semSet). */
    if (slot && !(fresh && isShorthand))
      irEmit(b->block, irStore(slot, isFor ? boundV : hidden));
  } else {
    /* while: kondisi umum; tanpa kondisi = loop tanpa henti. */
    irEmit(b->block, irJump(condBlock));
    b->block = condBlock;

    IRValue *c = a->loop.condition >= 0 ? buildNode(b, a->loop.condition) : NULL;
    if (!c) c = irBoolean(1, boolType());
    irEmit(b->block, irBranch(c, bodyBlock, endBlock));

    b->block = bodyBlock;
    buildBlock(b, a->loop.body);
    irEmit(b->block, irJump(condBlock));

    b->block = endBlock;
  }
  g_loop = ctx.parent;
}

static void buildCase(IRBuilder *b, const AstNode *a) {
  IRValue *subject = buildNode(b, a->asCase.subject);

  IRBlock *endBlock = newBlock(b, "case.end");
  IRBlock *nextBlock = NULL;

  for (int i = 0; i < a->asCase.length; i++) {
    struct AstCaseEntry *entry = &a->asCase.entries[i];
    IRBlock *bodyBlock = newBlock(b, "case.body");
    nextBlock = (entry->wildcard || entry->pattern < 0) ? NULL : newBlock(b, "case.next");

    if (entry->wildcard || entry->pattern < 0) {
      irEmit(b->block, irJump(bodyBlock));
    } else {
      IRValue *pattern = buildNode(b, entry->pattern);
      IRValue *eq = irTemp(boolType());
      irEmit(b->block, irBinary(IR_EQ, eq, subject, pattern));
      irEmit(b->block, irBranch(eq, bodyBlock, nextBlock));
    }

    irSetBlock(b->function, bodyBlock);
    b->block = bodyBlock;
    buildBlock(b, entry->body);
    irEmit(b->block, irJump(endBlock));

    if (nextBlock) b->block = nextBlock;
  }

  if (nextBlock) irEmit(b->block, irJump(endBlock));
  b->block = endBlock;
  (void)subject;
}

/* ==================== Expression ==================== */

static IRValue *buildNode(IRBuilder *b, int id) {
  if (id < 0 || !b->astRef || id >= b->astRef->length) return irNull(NULL);

  /* Cache operand per node id — identifier yang dipakai berulang
   * merujuk IRValue slot yang sama. */
  if (id < b->opLen && b->ops[id]) return b->ops[id];

  Node *node = b->astRef;
  AstNode *a = &node->ast[id];

  switch (a->type) {
  case NODE_NUMBER:
    /* number 64-bit: literal AST long long -> IR int64 (union sudah
     * int64_t, tidak ada perubahan tipe). */
    return cacheOp(b, id, irNumber(a->number.value, numberType()));
  case NODE_DECIMAL:
    return cacheOp(b, id,
                   irDecimal(a->decimal.value,
                             irTypeCreate(IR_TYPE_DECIMAL, "decimal", sizeof(double))));
  case NODE_BOOLEAN:
    return cacheOp(b, id, irBoolean(a->boolean.value, boolType()));
  case NODE_STRING:
    return cacheOp(b, id, irString(a->string.value, irTypeCreate(IR_TYPE_STRING, "string", 0)));
  case NODE_NULLABLE:
    return cacheOp(b, id, irNull(nullType()));

  case NODE_IDENTIFIER:
  case NODE_LITERAL_ID: {
    const char *name = a->type == NODE_IDENTIFIER ? a->identifier.name : a->string.value;
    IRValue *slot = scopeFind(b, name);
    if (!slot) slot = newLocal(b, name);
    /* Read-through string slot (design/str_memory.txt): bila slot
     * dideklarasikan 'string', variable membawa handle — baca slot
     * via op native IR_STRSLOT_GET (registry v3 memutuskan di runtime). */
    const char *declared = scopeTypeOf(b, name);
    if (declared && !strcmp(declared, "string")) {
      IRValue *res = irTemp(irTypeCreate(IR_TYPE_STRING, "string", 0));
      irEmit(b->block, irStrSlotGet(res, slot));
      return cacheOp(b, id, res);
    }
    return cacheOp(b, id, slot);
  }

  case NODE_BINARY: {
    const char *opStr = a->binary.op ? a->binary.op : "";
    IRValue *l = buildNode(b, a->binary.left);
    IRValue *r = buildNode(b, a->binary.right);

    /* Unary NOT direpresentasikan sebagai binary `!` dengan right < 0. */
    if (!strcmp(opStr, "!") && a->binary.right < 0) {
      IRValue *res = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, res, l));
      return res;
    }

    /* && / || diturunkan menjadi branch + blok (short-circuit).
     * Konvensi phi-less: res di-store default dulu, lalu dijalankan
     * store kedua di jalur yang dipilih. */
    if (!strcmp(opStr, "&&") || !strcmp(opStr, "||")) {
      IRBlock *rhsBlock = newBlock(b, "log.rhs");
      IRBlock *endBlock = newBlock(b, "log.end");
      IRValue *res = irTemp(boolType());
      bool isAnd = opStr[0] == '&';

      /* Short-circuit native: branch langsung pada truthiness kiri;
       * mesin mengevaluasi valueTruthy(machineGet(...)). */
      irEmit(b->block, irStore(res, irBoolean(isAnd ? 0 : 1, boolType())));
      if (isAnd)
        irEmit(b->block, irBranch(l, rhsBlock, endBlock));
      else
        irEmit(b->block, irBranch(l, endBlock, rhsBlock));

      b->block = rhsBlock;
      IRValue *notR = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, notR, r));
      IRValue *rTruthy = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, rTruthy, notR));
      irEmit(b->block, irStore(res, rTruthy));
      irEmit(b->block, irJump(endBlock));

      b->block = endBlock;
      return res;
    }

    IROpcode op = opcodeFor(opStr);
    if (op == IR_NOP) return irNull(NULL);
    return emitBinary(b, op, l, r);
  }

  case NODE_ARRAY: {
    IRValue *arr = irTemp(irTypeCreate(IR_TYPE_ARRAY, "array", 0));
    IRValue *count = irNumber(a->array.length, numberType());
    irEmit(b->block, irAlloc(arr, irTypeCreate(IR_TYPE_ARRAY, "array", 0), count, 0));

    for (int i = 0; i < a->array.length; i++) {
      IRValue *item = buildNode(b, a->array.elements[i]);
      IRValue *idx = irNumber(i, numberType());
      irEmit(b->block, irIndexSet(arr, idx, item));
    }
    return cacheOp(b, id, arr);
  }

  case NODE_OBJECT: {
    IRValue *obj = irTemp(irTypeCreate(IR_TYPE_OBJECT, "object", 0));
    irEmit(b->block, irAlloc(obj, irTypeCreate(IR_TYPE_OBJECT, "object", 0), NULL, 1));

    for (int i = 0; i < a->object.length; i++) {
      const char *key = nodeName(b->astRef, a->object.entries[i].key);
      /* Shorthand {name}: key == value node id → load variable bernama sama. */
      int entryValue = a->object.entries[i].value;
      IRValue *val;
      if (a->object.entries[i].key == entryValue) {
        val = scopeFind(b, key);
        if (!val) val = newLocal(b, key);
      } else {
        val = buildNode(b, entryValue);
      }
      if (key && val) irEmit(b->block, irMemberSet(obj, key, val));
    }
    return cacheOp(b, id, obj);
  }

  case NODE_MEMBER: {
    IRValue *obj = buildNode(b, a->member.object);
    const char *key = nodeName(b->astRef, a->member.member);
    IRValue *res = irTemp(nullType());
    irEmit(b->block, irMemberGet(res, obj, key ? key : ""));
    return res;
  }

  case NODE_SUBSCRIPT: {
    IRValue *arr = buildNode(b, a->subscript.posId);
    IRValue *idx = buildNode(b, a->subscript.index);
    IRValue *res = irTemp(nullType());
    irEmit(b->block, irIndexGet(res, arr, idx));
    return res;
  }

  case NODE_CALL: {
    AstNode *calleeNode = &node->ast[a->call.callee];
    /* sizeof(TypeName) — argumen nama tipe, bukan value: trampoline ke
     * interpretCall yang meng-intercept sizeof (pola method call). */
    const char *calleeName = (calleeNode->type == NODE_IDENTIFIER)
                                 ? calleeNode->identifier.name
                                 : (calleeNode->type == NODE_LITERAL_ID)
                                       ? calleeNode->string.value
                                       : NULL;
    bool isSizeof = calleeName && !strcmp(calleeName, "sizeof");
    /* new/del — type-driven memory: arg pertama `new` nama tipe, tak
     * bisa dievaluasi sebagai ekspresi. Trampoline ke interpretCall. */
    bool isMemory = calleeName && (!strcmp(calleeName, "new") || !strcmp(calleeName, "del"));
    if (isSizeof || isMemory || calleeNode->type == NODE_MEMBER ||
        calleeNode->type == NODE_SUBSCRIPT) {
      /* Method call (arr.push(x), s.split(","), obj.fn()): dispatch
       * method + write-back ada di interpreter, jadi trampoline seluruh
       * call node (sejajar interpretCall). sizeof juga: argumennya
       * nama tipe, tidak bisa dievaluasi sebagai ekspresi. */
      IRInstruction *in = irInterp(id, nullType());
      IRValue *res = in->result;
      irEmit(b->block, in);
      return res;
    }
    IRValue *callee = buildNode(b, a->call.callee);
    IRValue **args = NULL;
    if (a->call.length > 0) {
      args = gccalloc((size_t)a->call.length, sizeof(IRValue *));
      if (!args) return irNull(NULL);
      for (int i = 0; i < a->call.length; i++)
        args[i] = buildNode(b, a->call.args[i]);
    }
    IRValue *res = irTemp(nullType());
    irEmit(b->block, irCall(res, callee, args, (size_t)a->call.length));
    return res;
  }

  case NODE_UPDATE: {
    const char *name = nodeName(b->astRef, a->update.target);
    IRValue *slot = scopeFind(b, name);
    if (!slot && name) slot = newLocal(b, name);
    if (!slot) return irNull(NULL);

    if (a->update.value >= 0) {
      /* Compound assignment: x += v, -=, *=, /=, %=. Operator dasar
       * diambil dari karakter pertama ("+=" -> "+"). */
      char baseOp[2] = {a->update.op ? a->update.op[0] : '\0', '\0'};
      IROpcode op = opcodeFor(baseOp);
      if (op == IR_NOP) return irNull(NULL);
      IRValue *rhs = buildNode(b, a->update.value);
      if (!rhs) return irNull(NULL);
      IRValue *next = irTemp(slot->type);
      irEmit(b->block, irBinary(op, next, slotValue(slot), rhs));
      irEmit(b->block, irStore(slot, next));
      return next;
    }

    IRValue *one = irNumber(1, numberType());
    IRValue *next = irTemp(slot->type);
    if (a->update.op && !strcmp(a->update.op, "++")) {
      irEmit(b->block, irBinary(IR_ADD, next, slotValue(slot), one));
    } else {
      IRValue *neg = irTemp(numberType());
      irEmit(b->block, irUnary(IR_NEG, neg, one));
      irEmit(b->block, irBinary(IR_ADD, next, slotValue(slot), neg));
    }
    irEmit(b->block, irStore(slot, next));
    return a->update.prefix ? next : slotValue(slot);
  }

  case NODE_STRING_INTERP: {
    /* Rangkaian concat IR_ADD bertipe string. */
    IRValue *acc = irString("", irTypeCreate(IR_TYPE_STRING, "string", 0));
    for (int i = 0; i < a->stringInterp.length; i++) {
      IRValue *part = buildNode(b, a->stringInterp.parts[i]);
      IRValue *res = irTemp(irTypeCreate(IR_TYPE_STRING, "string", 0));
      irEmit(b->block, irBinary(IR_ADD, res, acc, part));
      acc = res;
    }
    return acc;
  }

  case NODE_THEN: {
    /* cond -> result : result hanya dievaluasi saat cond truthy;
     * selain itu hasilnya null (store default sebelum branch). */
    IRValue *cond = buildNode(b, a->then.condition);
    IRBlock *resBlock = newBlock(b, "then.res");
    IRBlock *endBlock = newBlock(b, "then.end");
    IRValue *res = irTemp(nullType());

    IRValue *truthy = irTemp(boolType());
    IRValue *notCond = irTemp(boolType());
    irEmit(b->block, irUnary(IR_NOT, notCond, cond));
    irEmit(b->block, irUnary(IR_NOT, truthy, notCond));
    irEmit(b->block, irStore(res, irNull(nullType())));
    irEmit(b->block, irBranch(truthy, resBlock, endBlock));

    b->block = resBlock;
    IRValue *val = buildNode(b, a->then.result);
    irEmit(b->block, irStore(res, val ? val : irNull(nullType())));
    irEmit(b->block, irJump(endBlock));

    b->block = endBlock;
    return res;
  }

  case NODE_FALLBACK: {
    /* primary | fallback : primary dievaluasi dulu; bila truthy dipakai,
     * bila tidak fallback dievaluasi. */
    IRValue *primary = buildNode(b, a->fallback.primary);
    IRBlock *keepBlock = newBlock(b, "fb.keep");
    IRBlock *fbBlock = newBlock(b, "fb.alt");
    IRBlock *endBlock = newBlock(b, "fb.end");
    IRValue *res = irTemp(nullType());

    IRValue *notPrimary = irTemp(boolType());
    irEmit(b->block, irUnary(IR_NOT, notPrimary, primary));
    IRValue *truthy = irTemp(boolType());
    irEmit(b->block, irUnary(IR_NOT, truthy, notPrimary));
    irEmit(b->block, irStore(res, irNull(nullType())));
    irEmit(b->block, irBranch(truthy, keepBlock, fbBlock));

    b->block = fbBlock;
    IRValue *fb = buildNode(b, a->fallback.fallback);
    irEmit(b->block, irStore(res, fb ? fb : irNull(nullType())));
    irEmit(b->block, irJump(endBlock));

    b->block = keepBlock;
    irEmit(b->block, irStore(res, primary));
    irEmit(b->block, irJump(endBlock));

    b->block = endBlock;
    return res;
  }

  default:
    return irNull(NULL);
  }
}

/* ==================== Statement dispatcher ==================== */

static IRValue *buildStatementValue(IRBuilder *b, int id) {
  if (id < 0 || !b->astRef || id >= b->astRef->length) return NULL;
  Node *node = b->astRef;
  AstNode *a = &node->ast[id];

  switch (a->type) {
  case NODE_ASSIGN:
    buildAssign(b, node, a, id);
    return NULL;
  case NODE_CONDITIONAL_ASSIGN:
    buildConditionalAssign(b, node, a);
    return NULL;
  case NODE_ANNOTATION:
    buildAnnotation(b, node, a);
    return NULL;
  case NODE_PRINT:
    buildPrint(b, node, a);
    return NULL;
  case NODE_RETURN:
    buildReturn(b, node, a);
    return NULL;
  case NODE_BLOCK:
    scopePush(b);
    buildBlock(b, id);
    scopePop(b);
    return NULL;
  case NODE_IF:
    buildIf(b, node, a);
    return NULL;
  case NODE_LOOP:
    buildLoop(b, node, a);
    return NULL;
  case NODE_BREAK:
    buildBreak();
    return NULL;
  case NODE_CONTINUE:
    buildContinue();
    return NULL;
  case NODE_CASE:
    buildCase(b, a);
    return NULL;
  case NODE_FUNCTION_DECL:
    buildFunctionDecl(b, node, a);
    return NULL;
  case NODE_STRUCT_DECL:
    /* Struktur = kontrak type: trampoline ke interpreter agar
     * layout terdaftar di analyzer registry (validasi struct-first
     * juga berlaku di jalur IR). */
    irEmit(b->block, irInterp(id, nullType()));
    return NULL;
  case NODE_ENUM_DECL:
    /* Enum (design/enum.txt): trampoline ke interpreter — konstanta
     * member dan object nama enum di-bind ke env. */
    irEmit(b->block, irInterp(id, nullType()));
    return NULL;
  case NODE_CLASS_DECL:
    /* Class = presentation di atas struct (design/new_class.txt).
     * Trampoline yang sama: method dalam body ikut tereksekusi via
     * interpretNode → statement dispatcher. */
    irEmit(b->block, irInterp(id, nullType()));
    return NULL;
  case NODE_MOD:
  case NODE_ASYNC:
  case NODE_AWAIT:
    /* Import/export/namespace + async/await butuh runtime penuh
     * (module loader, export policy, event loop, stdlib registry).
     * Trampoline ke interpretNode: payload call.count = AST node id. */
    irEmit(b->block, irInterp(id, nullType()));
    return NULL;
  case NODE_MEMBER_ASSIGN: {
    /* obj.field = v / obj["k"] = v / arr[i] = v */
    AstNode *target = &node->ast[a->memberAssign.target];
    IRValue *value = buildNode(b, a->memberAssign.value);
    if (!value) return NULL;

    if (target->type == NODE_MEMBER) {
      IRValue *obj = buildNode(b, target->member.object);
      const char *key = nodeName(node, target->member.member);
      if (obj && key) irEmit(b->block, irMemberSet(obj, key, value));
      return NULL;
    }
    if (target->type == NODE_SUBSCRIPT) {
      IRValue *base = buildNode(b, target->subscript.posId);
      IRValue *idx = buildNode(b, target->subscript.index);
      irEmit(b->block, irIndexSet(base, idx, value));
      return NULL;
    }
    return NULL;
  }
  default:
    /* Statement berbentuk expression (call, update, binary, dsb). */
    return buildNode(b, id);
  }
}

/* ==================== Entry point ==================== */

IRModule *rewrite(Node *node, int root, IRModule *ir) {
  if (!node || node->length <= 0 || !ir) return NULL;

  if (root < 0 || root >= node->length) {
    root = -1;
    for (int i = 0; i < node->length; i++) {
      if (node->ast[i].type == NODE_PROGRAM) {
        root = i;
        break;
      }
    }
    if (root < 0) return NULL;
  }

  IRBuilder b;
  builderInit(&b, node, ir);
  buildProgram(&b, node, root);
  return ir;
}
