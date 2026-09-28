#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite_flow.c — control flow (AST -> IR)
 *
 * If/loop/case diturunkan menjadi blok + IR_BRANCH/IR_JUMP.
 * Break/continue lewat konteks loop global (g_loop, tidak rekursif
 * antar builder). Walker astUsesIdentifier memakai bitmap seen
 * agar subexpression yang sama tidak dikunjungi dua kali — hasil
 * konservatif: node yang belum dikenal dianggap mereferensikan.
 * ============================================================ */

/* ==================== Loop context (break/continue) ==================== */

static LoopCtx *g_loop = NULL;

void buildBreak(void) {
  if (g_loop && g_loop->breakTarget) irEmit(g_loop->b->block, irJump(g_loop->breakTarget));
}

void buildContinue(void) {
  if (g_loop && g_loop->continueTarget) irEmit(g_loop->b->block, irJump(g_loop->continueTarget));
}

/* ==================== If ==================== */

void buildIf(IRBuilder *b, Node *node, const AstNode *a) {
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

/* ==================== Loop ==================== */

void buildLoop(IRBuilder *b, Node *node, const AstNode *a) {
  bool isFor = a->loop.kind && !strcmp(a->loop.kind, "for");
  bool isRev = a->loop.kind && !strcmp(a->loop.kind, "rev");

  /* Binding loop implisit (design loop): variable belum ada = milik
   * loop — di-unbind setelah loop (print(i) => undefined). Variable
   * yang sudah dideklarasikan (i = 0; for i < 10) tetap hidup dengan
   * nilai akhirnya. Probe env SEKALI di sini (runtime = sumber
   * kebenaran), hasilnya jadi syarat unbind di loop.end. */
  IRValue *absentProbe = NULL;
  const char *probeName = NULL;
  if (isFor || isRev) {
    const AstNode *cn0 = a->loop.condition >= 0 ? &b->astRef->ast[a->loop.condition] : NULL;
    if (cn0 && (cn0->type == NODE_IDENTIFIER || cn0->type == NODE_LITERAL_ID))
      probeName = cn0->type == NODE_IDENTIFIER ? cn0->identifier.name : cn0->string.value;
    else if (cn0 && cn0->type == NODE_BINARY) {
      const AstNode *l0 = cn0->binary.left >= 0 ? &b->astRef->ast[cn0->binary.left] : NULL;
      if (l0 && (l0->type == NODE_IDENTIFIER || l0->type == NODE_LITERAL_ID))
        probeName = l0->type == NODE_IDENTIFIER ? l0->identifier.name : l0->string.value;
      else {
        const AstNode *r0 = cn0->binary.right >= 0 ? &b->astRef->ast[cn0->binary.right] : NULL;
        if (r0 && (r0->type == NODE_IDENTIFIER || r0->type == NODE_LITERAL_ID))
          probeName = r0->type == NODE_IDENTIFIER ? r0->identifier.name : r0->string.value;
      }
    }
    if (probeName) {
      absentProbe = irTemp(boolType());
      irEmit(b->block, irProbeAbsent(absentProbe, irGlobal(nullType(), probeName)));
    }
  }

  /* old_loop/mixed (design/next_loop.txt): `for i=0; i < 10: ...` /
   * `rev i=10; i > 0 { ... }` — init dieksekusi sekali SEBELUM loop
   * (menulis slot variable agar counter range membacanya). */
  if (a->loop.init >= 0) buildStatementValue(b, a->loop.init);

  bool isWhile = !isFor && !isRev;
  /* Variable loop hanya "dipakai" bila BODY mereferensikannya — kondisi
   * header di level IR selalu memakai counter internal (slot tidak
   * pernah dibaca per iterasi), jadi walk body saja. Bila body tidak
   * menyentuh variable: sinkronisasi slot <- counter per iterasi
   * di-skip — hemat semSetCanon per iterasi untuk loop besar
   * ber-body kosong/tanpa akses variable. Final store di loop.end
   * tetap (1x per loop) supaya pembacaan variable SETELAH loop tetap
   * sejajar interpreter. */
  const char *loopVarName = NULL;
  {
    const AstNode *cn = a->loop.condition >= 0 ? &b->astRef->ast[a->loop.condition] : NULL;
    if (cn && (cn->type == NODE_IDENTIFIER || cn->type == NODE_LITERAL_ID))
      loopVarName = cn->type == NODE_IDENTIFIER ? cn->identifier.name : cn->string.value;
    else if (cn && cn->type == NODE_BINARY) {
      const AstNode *l = cn->binary.left >= 0 ? &b->astRef->ast[cn->binary.left] : NULL;
      const AstNode *r = cn->binary.right >= 0 ? &b->astRef->ast[cn->binary.right] : NULL;
      if (l && (l->type == NODE_IDENTIFIER || l->type == NODE_LITERAL_ID))
        loopVarName = l->type == NODE_IDENTIFIER ? l->identifier.name : l->string.value;
      else if (r && (r->type == NODE_IDENTIFIER || r->type == NODE_LITERAL_ID))
        loopVarName = r->type == NODE_IDENTIFIER ? r->identifier.name : r->string.value;
    }
  }
  bool usesLoopVar =
      isWhile || !loopVarName || astUsesIdentifier(b->astRef, a->loop.body, loopVarName);

  /* Empty-body peeling (profil: `for i < 100000 {}` = ~97% waktu di
   * execute padahal body kosong): loop dengan body BLOCK TANPA
   * statement punya iterasi yang tidak berefek — blok body hanya
   * `jump step`. Counting-loop (for/rev, kondisi binary op, bound
   * literal, variable BELUM ter-bind saat rewrite) bisa dipecah:
   * iterasi n dihitung dengan aturan SEMANTIK start=0 (sejajar
   * interpretForLoop: fresh ident-kiri mulai dari 0):
   *   for `<` : n = max(0, bound - 0)
   *   for `<=`: n = max(0, bound - 0 + 1)
   *   rev `>` : n = max(0, 0 - bound) = 0 (fresh rev ident-kiri
   *             mulai bound -> tak pernah > bound... sejajar
   *             interpreter: fresh rev ident-kiri start=bound, jadi
   *             `rev i > 0` fresh = 0 iterasi)
   * Karena fresh rev ident-kiri memulai counter dari bound (nilai
   * pertama yang GAGAL kondisi), hasilnya selalu 0 iterasi untuk
   * bentuk `>`; `>=` juga 0 (mulai di bound, kondisi langsung false).
   * Efek akhir loop tetap di-emit: for -> variable = bound (sejajar
   * semSet terakhir interpreter), lalu unbind implisit bila probe.
   * Semua efek samping loop (store per-iterasi variable implisit)
   * aman di-skip: binding implisit tak pernah terjadi, binding
   * eksplisit tertimpa final store.
   * Loop lain (body berisi statement, while, bound non-literal,
   * variable sudah ter-bind, rev ident-kanan) jalan jalur normal. */
  bool emptyBody = a->loop.body >= 0 && a->loop.body < b->astRef->length &&
                   b->astRef->ast[a->loop.body].type == NODE_BLOCK &&
                   b->astRef->ast[a->loop.body].block.length == 0;
  bool countable = (isFor || isRev) && emptyBody && loopVarName &&
                   a->loop.init < 0 && /* init punya efek samping — jangan peel */
                   a->loop.condition >= 0 &&
                   b->astRef->ast[a->loop.condition].type == NODE_BINARY;

  if (countable) {
    const AstNode *cn = &b->astRef->ast[a->loop.condition];
    const char *opStr = cn->binary.op ? cn->binary.op : "";
    bool valid = !strcmp(opStr, "<") || !strcmp(opStr, "<=") ||
                 !strcmp(opStr, ">") || !strcmp(opStr, ">=");
    /* Peeling hanya bentuk ident-kiri: `for i < bound` / `rev i > bound`.
    * Bentuk ident-kanan (`for 10 < i`) punya aturan start berbeda. */
    const AstNode *ident = cn->binary.left >= 0 ? &b->astRef->ast[cn->binary.left] : NULL;
    bool identLeft = ident && (ident->type == NODE_IDENTIFIER || ident->type == NODE_LITERAL_ID);
    if (!identLeft) valid = false;

    const AstNode *boundNode = cn->binary.right >= 0 ? &b->astRef->ast[cn->binary.right] : NULL;
    bool boundLiteral = boundNode && boundNode->type == NODE_NUMBER;
    bool varFresh = loopVarName ? !scopeFind(b, loopVarName) : false;

    if (valid && boundLiteral && varFresh) {
      /* TANPA iterasi sama sekali — sejajar interpreter:
       *   for  fresh ident-kiri: counter mulai 0, semSet slot per
       *        iterasi lalu final store = bound, TAPI... seluruh
       *        efek sampingnya adalah bind/unbind variable implisit
       *        yang tidak diamati body kosong — kecuali variable
       *        dibaca SETELAH loop. Untuk itu final store tetap
       *        di-emit (variable = bound) sebelum unbind. */
      IRValue *slot = scopeFind(b, loopVarName);
      if (!slot) slot = newLocal(b, loopVarName);
      if (isFor)
        irEmit(b->block, irStore(slot, irNumber(boundNode->number.value, numberType())));
      /* rev ident-kiri fresh: counter mulai = bound -> kondisi
       * langsung gagal -> 0 iterasi, variable tak pernah di-set
       * (sejajar interpretRevLoop) — tanpa final store. */
      if (absentProbe)
        irEmit(b->block, irUnbind(slot, absentProbe));
      return; /* loop terpecah: tanpa iterasi, tanpa blok body */
    }
  }

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
    bool neg1Select = false; /* aturan current == -1 -> bound (for eksplisit ident-left) */

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
      if (!start && fresh && isRev) start = isShorthand ? irNumber(-1, numberType()) : boundV;
      if (!start) start = irNumber(0, numberType());
      irEmit(b->block, irStore(hidden, start));
    }

    irEmit(b->block, irJump(condBlock));
    b->block = condBlock;

    IRValue *cmp = irTemp(boolType());
    irEmit(b->block, irBinary(cmpOp, cmp, hidden, boundV));
    irEmit(b->block, irBranch(cmp, bodyBlock, endBlock));

    b->block = bodyBlock;
    /* Sinkronkan variable user ke counter internal di awal iterasi.
     * Skip saat body/condition tidak mereferensikan variable — bind
     * per-iterasi percuma (mesin tidak pernah membacanya). */
    if (slot && usesLoopVar) irEmit(b->block, irStore(slot, hidden));
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
    if (slot && !(fresh && isShorthand)) irEmit(b->block, irStore(slot, isFor ? boundV : hidden));

    /* Unbind binding implisit (design loop) — bersyarat probe awal. */
    if (absentProbe && slot)
      irEmit(b->block, irUnbind(slot, absentProbe));
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

  /* while: unbind tidak berlaku (tanpa variable loop implisit). */
  (void)node;
  g_loop = ctx.parent;
}

/* ==================== Case ==================== */

void buildCase(IRBuilder *b, const AstNode *a) {
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

/* ==================== Walker referensi identifier ==================== */

/* ---- Deteksi referensi identifier di subtree AST ----
 * Untuk optimasi loop: sinkronisasi variable user <- counter internal
 * per iterasi hanya berarti bila body/condition benar-benar membaca
 * (atau menulis) variable itu. Walker konservatif: tipe node yang
 * belum dikenal dianggap "mereferensikan" agar optimasi tidak pernah
 * mengubah semantik. */

typedef struct {
  const Node *ast;
  const char *name;
  uint64_t *seen;
  int seenCap;
  bool found;
  int depth;
} IdUseCtx;

static void astIdUseWalk(IdUseCtx *c, int id) {
  if (c->found || !c->ast || id < 0 || id >= c->ast->length) return;
  if (id < c->seenCap && (c->seen[id >> 6] >> (id & 63)) & 1) return;
  if (id < c->seenCap) c->seen[id >> 6] |= (uint64_t)1 << (id & 63);
  if (++c->depth > 4096) {
    c->depth--;
    return;
  }

  const AstNode *a = &c->ast->ast[id];
  if (a->type == NODE_IDENTIFIER) {
    if (a->identifier.name && strcmp(a->identifier.name, c->name) == 0) c->found = true;
  } else if (a->type == NODE_LITERAL_ID) {
    if (a->string.value && strcmp(a->string.value, c->name) == 0) c->found = true;
  }

  if (!c->found) {
    switch (a->type) {
    case NODE_ASSIGN:
      astIdUseWalk(c, a->assign.target);
      astIdUseWalk(c, a->assign.value);
      break;
    case NODE_CONDITIONAL_ASSIGN:
      astIdUseWalk(c, a->conditionalAssign.target);
      astIdUseWalk(c, a->conditionalAssign.value);
      break;
    case NODE_BINARY:
      astIdUseWalk(c, a->binary.left);
      astIdUseWalk(c, a->binary.right);
      break;
    case NODE_MEMBER:
      astIdUseWalk(c, a->member.object);
      astIdUseWalk(c, a->member.member);
      break;
    case NODE_SUBSCRIPT:
      astIdUseWalk(c, a->subscript.posId);
      astIdUseWalk(c, a->subscript.index);
      break;
    case NODE_CALL: {
      astIdUseWalk(c, a->call.callee);
      for (int k = 0; !c->found && k < a->call.length; k++)
        astIdUseWalk(c, a->call.args[k]);
      break;
    }
    case NODE_PRINT:
      for (int k = 0; !c->found && k < a->print.length; k++)
        astIdUseWalk(c, a->print.args[k]);
      break;
    case NODE_BLOCK:
    case NODE_PROGRAM:
      for (int k = 0; !c->found && k < a->block.length; k++)
        astIdUseWalk(c, a->block.statements[k]);
      break;
    case NODE_IF:
      astIdUseWalk(c, a->asIf.condition);
      astIdUseWalk(c, a->asIf.thenBlock);
      astIdUseWalk(c, a->asIf.elseBlock);
      break;
    case NODE_LOOP:
      astIdUseWalk(c, a->loop.condition);
      astIdUseWalk(c, a->loop.body);
      break;
    case NODE_FUNCTION_DECL:
      for (int k = 0; !c->found && k < a->function.paramLength; k++)
        astIdUseWalk(c, a->function.params[k]);
      astIdUseWalk(c, a->function.body);
      break;
    case NODE_RETURN:
      astIdUseWalk(c, a->asReturn.expression);
      break;
    case NODE_ANNOTATION:
      astIdUseWalk(c, a->annotation.value);
      break;
    case NODE_UPDATE:
      astIdUseWalk(c, a->update.target);
      astIdUseWalk(c, a->update.value);
      break;
    case NODE_MEMBER_ASSIGN:
      astIdUseWalk(c, a->memberAssign.target);
      astIdUseWalk(c, a->memberAssign.value);
      break;
    case NODE_STRING_INTERP:
      for (int k = 0; !c->found && k < a->stringInterp.length; k++)
        astIdUseWalk(c, a->stringInterp.parts[k]);
      break;
    case NODE_ARRAY:
      for (int k = 0; !c->found && k < a->array.length; k++)
        astIdUseWalk(c, a->array.elements[k]);
      break;
    case NODE_OBJECT:
      for (int k = 0; !c->found && k < a->object.length; k++) {
        astIdUseWalk(c, a->object.entries[k].key);
        astIdUseWalk(c, a->object.entries[k].value);
      }
      break;
    case NODE_CASE:
      astIdUseWalk(c, a->asCase.subject);
      for (int k = 0; !c->found && k < a->asCase.length; k++) {
        astIdUseWalk(c, a->asCase.entries[k].pattern);
        astIdUseWalk(c, a->asCase.entries[k].body);
      }
      break;
    default:
      /* Tipe tak dikenal / tanpa child integer: anggap mereferensikan
       * (konservatif — optimasi dilewatkan, semantik aman). */
      switch (a->type) {
      case NODE_NUMBER:
      case NODE_DECIMAL:
      case NODE_BOOLEAN:
      case NODE_STRING:
      case NODE_INLINE_COMMENT:
      case NODE_BLOCK_COMMENT:
        break; /* literal & komentar: aman di-skip */
      default:
        c->found = true;
        break;
      }
      break;
    }
  }
  c->depth--;
}

/* true bila subtree id mereferensikan identifier `name` (NULL => false). */
bool astUsesIdentifier(const Node *ast, int id, const char *name) {
  if (!ast || !name || id < 0 || id >= ast->length) return false;
  int words = (ast->length + 63) / 64;
  uint64_t *seen = gccalloc((size_t)words, sizeof(uint64_t));
  if (!seen) return true; /* gagal alokasi => konservatif */
  IdUseCtx c = {
      .ast = ast, .name = name, .seen = seen, .seenCap = ast->length, .found = false, .depth = 0};
  astIdUseWalk(&c, id);
  return c.found;
}
