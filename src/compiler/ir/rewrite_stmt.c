#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite_stmt.c — statement (AST -> IR)
 *
 * buildProgram menurunkan NODE_PROGRAM menjadi IRFunction "main";
 * fungsi deklarasi menjadi IRFunction tersendiri. Assign menangani
 * kontrak type (analyzer check + scope type), new Contract, dan
 * write-through string slot. buildStatementValue adalah dispatcher
 * statement -> builder; node runtime penuh (struct/enum/class/mod/
 * async) di-trampoline ke interpreter via IR_INTERP.
 * ============================================================ */

/* ==================== Program & fungsi ==================== */

/* Statement top-level berupa call `main()` tanpa argumen? Dipakai
 * untuk guard auto-run: main yang sudah dipanggil eksplisit tidak
 * boleh dipanggil dua kali (main1.rp: `1` lalu `null1`). */
static bool topLevelIsMainCall(Node *node, int id) {
  if (id < 0 || id >= node->length) return false;
  AstNode *a = &node->ast[id];
  if (a->type != NODE_CALL || a->call.length != 0) return false;
  int calleeId = a->call.callee;
  if (calleeId < 0 || calleeId >= node->length) return false;
  AstNode *c = &node->ast[calleeId];
  const char *name = NULL;
  if (c->type == NODE_IDENTIFIER)
    name = c->identifier.name;
  else if (c->type == NODE_LITERAL_ID)
    name = c->string.value;
  return name && !strcmp(name, "main");
}

void buildProgram(IRBuilder *b, Node *node, int root) {
  IRFunction *fn = irFunctionCreate(b->module, "main", NULL);
  b->function = fn;
  b->block = irBlockCreate(fn, "entry");

  bool hasMainDecl = false;
  bool mainCalled = false;
  for (AstDeclaration *d = node->ast[root].program.declarations; d; d = d->next) {
    AstNode *a = &node->ast[d->nodeId];
    if (a->type == NODE_FUNCTION_DECL && a->function.name >= 0 && a->function.name < node->length &&
        node->ast[a->function.name].type == NODE_IDENTIFIER &&
        !strcmp(node->ast[a->function.name].identifier.name, "main") &&
        a->function.paramLength == 0)
      hasMainDecl = true;
    if (topLevelIsMainCall(node, d->nodeId)) mainCalled = true;
    buildStatementValue(b, d->nodeId);
  }

  /* `main()` otomatis (design class, entry point): deklarasi main
   * zero-param yang tidak dipanggil eksplisit di-call implicit di
   * akhir blok program — satu frame eksekusi, identik dengan call
   * manual (free variable + IR_CHECK + stdlib semua terjangkau).
   * main berparameter: bukan entry point, tidak di-call. */
  if (hasMainDecl && !mainCalled)
    irEmit(b->block, irCall(NULL, irGlobal(nullType(), "main"), NULL, 0));

  irEmit(b->block, irReturn(irNull(NULL)));
}

void buildFunctionDecl(IRBuilder *b, Node *node, const AstNode *a) {
  const char *name = nodeName(node, a->function.name);

  IRFunction *savedFn = b->function;
  IRBlock *savedBlock = b->block;
  ScopeMap *savedScope = b->scopes;

  /* Return-type annotation (`foo(): string { }`): catat nama tipe di
   * IRFunction — di-enforce saat eksekusi (IR_CHECK di buildReturn;
   * void oleh IR_RETURN di execute.c). Kind placeholder, nama yang
   * dipakai validator. */
  IRType *retT = NULL;
  if (a->function.returnType >= 0) {
    const char *rtName = annotationTypeName(b, a->function.returnType);
    if (rtName && !strcmp(rtName, "void"))
      retT = irTypeCreate(IR_TYPE_VOID, "void", 0);
    else if (rtName && !strcmp(rtName, "object"))
      retT = irTypeCreate(IR_TYPE_OBJECT, "object", 0);
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
  if (b->block->last && b->block->last->op != IR_RETURN) irEmit(b->block, irReturn(irNull(NULL)));

  b->function = savedFn;
  b->block = savedBlock;
  b->scopes = savedScope;

  /* Bind fungsi ke env via IRFunction slot — dipakai execCall dan
   * trampoline interpretNode (callLoader/callHandler async, callback).
   * Slot IR_VALUE_FUNCTION menandakan "fungsi dengan nama ini". */
  if (name) {
    IRValue *slot = irFunctionValue(nullType(), name);
    irEmit(b->block, irStore(slot, irFunctionValue(nullType(), name)));
    /* `pub a() {}` (design fn): binding publik — export surface. */
    if (a->function.isPub) irEmit(b->block, irMarkPub(irGlobal(nullType(), name)));
  }
}

/* ==================== Assign ==================== */

void buildAssign(IRBuilder *b, Node *node, const AstNode *a, int id) {
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
        call->call.length >= 2 ? buildNode(b, call->call.args[1]) : irNumber(1, numberType());
    /* Bentuk 3-arg = calloc custom: elemSize ditunda ke eksekusi via
     * elemSize < 0 (payload = node id arg kedua); bila arg elemSize
     * literal, elemSize di-set langsung. */
    int elemSize = 0;
    if (call->call.length == 3) {
      AstNode *es = &b->astRef->ast[call->call.args[2]];
      if (es->type == NODE_NUMBER) {
        elemSize = (int)es->number.value;
      } else {
        elemSize = -call->call.args[2];
      }
    }
    IRType *ptrT = irTypeCreate(IR_TYPE_POINTER, ann ? ann : "ptr", sizeof(void *));
    IRValue *res = irTemp(ptrT);
    irEmit(b->block, irAlloc(res, ptrT, count, 1, elemSize));
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
      if (declared) irEmit(b->block, irCheckAt(value, declared, a->assign.value));
    }
  }

  /* nodeId = statement assign (bukan node value) — lokasi ConstError
   * menunjuk baris assignment, bukan ekspresi sisi kanan. */
  irEmit(b->block, irStoreAt(slot, value, a->assign.isConst, id));
  /* `pub x = 10` (design namespace): binding publik — export surface. */
  if (a->assign.isPub && name) irEmit(b->block, irMarkPub(irGlobal(nullType(), name)));
}

void buildConditionalAssign(IRBuilder *b, Node *node, const AstNode *a) {
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
    if (declared) irEmit(b->block, irCheckAt(value, declared, a->conditionalAssign.value));
    irEmit(b->block, irStore(slot, value));
  }
  irEmit(b->block, irJump(endBlock));

  b->block = endBlock;
}

void buildAnnotation(IRBuilder *b, Node *node, const AstNode *a) {
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
        call->call.length >= 2 ? buildNode(b, call->call.args[1]) : irNumber(1, numberType());
    /* Sejajar buildAssign: bentuk 3-arg calloc custom, elemSize
     * eksplisit atau ditunda (negasi node id). */
    int elemSize = 0;
    if (call->call.length == 3) {
      AstNode *es = &b->astRef->ast[call->call.args[2]];
      if (es->type == NODE_NUMBER) {
        elemSize = (int)es->number.value;
      } else {
        elemSize = -call->call.args[2];
      }
    }
    IRType *ptrT = irTypeCreate(IR_TYPE_POINTER, ann ? ann : "ptr", sizeof(void *));
    IRValue *res = irTemp(ptrT);
    irEmit(b->block, irAlloc(res, ptrT, count, 1, elemSize));
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

/* ==================== Print & return ==================== */

void buildPrint(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  /* Satu call dengan SEMUA args — print engine (print_format.c)
   * butuh daftar arg utuh untuk pola 2 (multi-arg), pola 3 (format
   * printf-style), dan pola 4 (stream target stdout/stderr). */
  int argc = a->print.length;
  IRValue **args = NULL;
  if (argc > 0) {
    args = gccalloc((size_t)argc, sizeof(IRValue *));
    if (!args) return;
    for (int i = 0; i < argc; i++)
      args[i] = buildNode(b, a->print.args[i]);
  }
  IRValue *callee = irFunctionValue(nullType(), "print");
  irEmit(b->block, irCall(irTemp(nullType()), callee, args, (size_t)argc));
}

void buildReturn(IRBuilder *b, Node *node, const AstNode *a) {
  (void)node;
  /* explicitReturn=false adalah expression statement (REPL semantic):
   * evaluasi untuk efek samping saja, JANGAN emit IR_RETURN — kalau
   * tidak, sisa program setelah statement ini jadi dead code. */
  if (!a->asReturn.explicitReturn) {
    if (a->asReturn.expression >= 0) buildNode(b, a->asReturn.expression);
    return;
  }
  IRValue *value =
      a->asReturn.expression >= 0 ? buildNode(b, a->asReturn.expression) : irNull(NULL);
  if (!value) value = irNull(NULL);
  /* Kontrak return-type (annotation non-void): validasi value terhadap
   * nama tipe saat eksekusi via IR_CHECK — analyzerCheckType menangani
   * scalar, struct terdaftar, dan "T[]" (nested), sejajar assignment
   * bertipe. Void ditangani sendiri oleh IR_RETURN (execute.c: nilai
   * non-null = TypeError). Return sintetis fall-through (buildFunctionDecl)
   * tidak lewat sini — sejajar interpreter yang tak mengecek fall-out. */
  if (b->function && b->function->return_type && b->function->return_type->name &&
      strcmp(b->function->return_type->name, "void") != 0)
    irEmit(b->block, irCheckFunctionAt(value, b->function->return_type->name,
                                       a->asReturn.expression, b->function->name));
  irEmit(b->block, irReturn(value));
}

void buildBlock(IRBuilder *b, int id) {
  if (id < 0 || id >= b->astRef->length) return;
  AstNode *a = &b->astRef->ast[id];
  /* Branch `else if` memproduksi node IF langsung (bukan NODE_BLOCK) —
   * baca union block tanpa cek type = garbage length -> segfault. */
  if (a->type != NODE_BLOCK) {
    buildStatementValue(b, id);
    return;
  }
  for (int i = 0; i < a->block.length; i++)
    buildStatementValue(b, a->block.statements[i]);
}

/* ==================== Statement dispatcher ==================== */

IRValue *buildStatementValue(IRBuilder *b, int id) {
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
     * interpretNode -> statement dispatcher. */
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
