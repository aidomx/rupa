#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite_expr.c — ekspresi (AST -> IR)
 *
 * buildNode menurunkan ekspresi menjadi IRValue (konstanta, slot,
 * temp hasil instruksi). Literal di-cache per node id; identifier
 * merujuk slot dari scope map. Binary aritmetika/komparasi lewat
 * emitBinary (dengan const-fold); &&/|| dan ternary/pipe diturunkan
 * menjadi blok + branch (phi-less: store default sebelum branch).
 * ============================================================ */

/* Kumpulkan segmen rantai pipe nested-kiri (parseBinary) menjadi
 * urutan source (kiri->kanan). Return jumlah segmen; segs diisi id AST. */
int pipeSegments(Node *node, int id, int *segs, int cap) {
  int nseg = 0;
  int cur = id;
  while (cur >= 0 && cur < node->length && node->ast[cur].type == NODE_FALLBACK && nseg < cap - 1) {
    segs[nseg++] = node->ast[cur].fallback.fallback;
    cur = node->ast[cur].fallback.primary;
  }
  if (cur >= 0 && cur < node->length && nseg < cap) segs[nseg++] = cur;
  for (int lo = 0; lo < nseg / 2; lo++) {
    int tmp = segs[lo];
    segs[lo] = segs[nseg - 1 - lo];
    segs[nseg - 1 - lo] = tmp;
  }
  return nseg;
}

IRValue *buildNode(IRBuilder *b, int id) {
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
    return cacheOp(
        b, id,
        irDecimal(a->decimal.value, irTypeCreate(IR_TYPE_DECIMAL, "decimal", sizeof(double))));
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
    irEmit(b->block, irAlloc(arr, irTypeCreate(IR_TYPE_ARRAY, "array", 0), count, 0, 0));

    for (int i = 0; i < a->array.length; i++) {
      IRValue *item = buildNode(b, a->array.elements[i]);
      IRValue *idx = irNumber(i, numberType());
      irEmit(b->block, irIndexSet(arr, idx, item));
    }
    return cacheOp(b, id, arr);
  }

  case NODE_OBJECT: {
    IRValue *obj = irTemp(irTypeCreate(IR_TYPE_OBJECT, "object", 0));
    irEmit(b->block, irAlloc(obj, irTypeCreate(IR_TYPE_OBJECT, "object", 0), NULL, 1, 0));

    for (int i = 0; i < a->object.length; i++) {
      const char *key = nodeName(b->astRef, a->object.entries[i].key);
      /* Shorthand {name}: key == value node id -> load variable bernama sama. */
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
    const char *calleeName = (calleeNode->type == NODE_IDENTIFIER)   ? calleeNode->identifier.name
                             : (calleeNode->type == NODE_LITERAL_ID) ? calleeNode->string.value
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
    /* Ternary Rupa: cond -> then | else — THEN di atas rantai
     * FALLBACK. Kondisi truthy -> evaluasi then (seluruh rantai),
     * bila tidak -> sisi else dievaluasi or-else per segmen
     * (bila result bukan rantai pipe: langsung nilai itu —
     * semantik THEN lama, hasil null saat cond falsy). */
    if (b->astRef->ast[a->then.result].type == NODE_FALLBACK) {
      int segs[64];
      int nseg = pipeSegments(b->astRef, a->then.result, segs, 64);
      IRValue *cond = buildNode(b, a->then.condition);
      IRBlock *thenBlock = newBlock(b, "then.take");
      IRBlock *elseBlock = newBlock(b, "then.else");
      IRBlock *endBlock = newBlock(b, "then.end");
      IRValue *res = irTemp(nullType());

      IRValue *truthy = irTemp(boolType());
      IRValue *notCond = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, notCond, cond));
      irEmit(b->block, irUnary(IR_NOT, truthy, notCond));
      irEmit(b->block, irStore(res, irNull(nullType())));
      irEmit(b->block, irBranch(truthy, thenBlock, elseBlock));

      b->block = thenBlock;
      IRValue *tv = buildNode(b, a->then.result);
      irEmit(b->block, irStore(res, tv ? tv : irNull(nullType())));
      irEmit(b->block, irJump(endBlock));

      b->block = elseBlock;
      irEmit(b->block, irStore(res, irNull(nullType())));
      for (int i = 1; i < nseg; i++) {
        IRValue *fbv = buildNode(b, segs[i]);
        IRBlock *okBlock = newBlock(b, "then.ok");
        IRBlock *nextBlock = newBlock(b, "then.next");
        IRValue *truthyFb = irTemp(boolType());
        IRValue *notFb = irTemp(boolType());
        irEmit(b->block, irUnary(IR_NOT, notFb, fbv));
        irEmit(b->block, irUnary(IR_NOT, truthyFb, notFb));
        irEmit(b->block, irStore(res, fbv ? fbv : irNull(nullType())));
        irEmit(b->block, irBranch(truthyFb, okBlock, nextBlock));
        b->block = okBlock;
        irEmit(b->block, irJump(endBlock));
        b->block = nextBlock;
      }
      irEmit(b->block, irJump(endBlock));

      b->block = endBlock;
      return res;
    }

    /* THEN polos: cond -> result : result hanya dievaluasi saat cond
     * truthy; selain itu hasilnya null (store default sebelum branch). */
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
    /* Rantai pipe nested kiri (parseBinary). Rantai >= 3 segmen =
     * ternary pipa c1 | v1 | c2 | v2 | ... | else (lazy, pasangan
     * kondisi->nilai); 2 segmen = or-else biasa (primary | fallback). */
    int segs[64];
    int nseg = pipeSegments(b->astRef, id, segs, 64);

    if (nseg < 3) {
      /* primary | fallback : primary dievaluasi dulu; bila truthy
       * dipakai, bila tidak fallback dievaluasi. */
      IRValue *primary = buildNode(b, nseg > 0 ? segs[0] : -1);
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
      IRValue *fb = buildNode(b, nseg > 1 ? segs[1] : -1);
      irEmit(b->block, irStore(res, fb ? fb : irNull(nullType())));
      irEmit(b->block, irJump(endBlock));

      b->block = keepBlock;
      irEmit(b->block, irStore(res, primary));
      irEmit(b->block, irJump(endBlock));

      b->block = endBlock;
      return res;
    }

    /* Cascade ternary: segmen THEN (cond -> val) — lengan berurutan,
     * nilai truthy pertama menang; segmen terakhir = else.
     * c1 -> v1 | c2 -> v2 | else. */
    if (b->astRef->ast[segs[0]].type == NODE_THEN) {
      IRValue *res = irTemp(nullType());
      IRBlock *endBlock = newBlock(b, "casc.end");
      irEmit(b->block, irStore(res, irNull(nullType())));
      for (int i = 0; i + 1 < nseg; i++) {
        IRValue *v = buildNode(b, segs[i]);
        IRBlock *okBlock = newBlock(b, "casc.ok");
        IRBlock *nextBlock = newBlock(b, "casc.next");
        IRValue *truthy = irTemp(boolType());
        IRValue *nv = irTemp(boolType());
        irEmit(b->block, irUnary(IR_NOT, nv, v));
        irEmit(b->block, irUnary(IR_NOT, truthy, nv));
        irEmit(b->block, irStore(res, v ? v : irNull(nullType())));
        irEmit(b->block, irBranch(truthy, okBlock, nextBlock));
        b->block = okBlock;
        irEmit(b->block, irJump(endBlock));
        b->block = nextBlock;
      }
      IRValue *lastv = buildNode(b, segs[nseg - 1]);
      irEmit(b->block, irStore(res, lastv ? lastv : irNull(nullType())));
      irEmit(b->block, irJump(endBlock));
      b->block = endBlock;
      return res;
    }

    /* Ternary pipa lazy: evaluasi kondisi k-> branch. Nilai hanya
     * dibangun di bloknya sendiri. Segmen ganjil terakhir = else. */
    IRValue *res = irTemp(nullType());
    IRBlock *endBlock = newBlock(b, "pipe.end");
    irEmit(b->block, irStore(res, irNull(nullType())));

    int i = 0;
    for (; i + 1 < nseg; i += 2) {
      IRValue *cond = buildNode(b, segs[i]);
      IRBlock *takeBlock = newBlock(b, "pipe.take");
      IRBlock *nextBlock = newBlock(b, "pipe.next");
      IRValue *truthy = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, truthy, cond));
      IRValue *take = irTemp(boolType());
      irEmit(b->block, irUnary(IR_NOT, take, truthy));
      irEmit(b->block, irBranch(take, takeBlock, nextBlock));

      b->block = takeBlock;
      IRValue *val = buildNode(b, segs[i + 1]);
      irEmit(b->block, irStore(res, val ? val : irNull(nullType())));
      irEmit(b->block, irJump(endBlock));

      b->block = nextBlock;
    }
    if (i < nseg) {
      IRValue *val = buildNode(b, segs[i]);
      irEmit(b->block, irStore(res, val ? val : irNull(nullType())));
    }
    irEmit(b->block, irJump(endBlock));
    b->block = endBlock;
    return res;
  }

  default:
    return irNull(NULL);
  }
}
