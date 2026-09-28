#include <rupa.h>

#include "rewrite_internal.h"

/* ============================================================
 * rewrite_builder.c — core builder (AST -> IR)
 *
 * Helper inti IRBuilder: inisialisasi, cache operand per node id,
 * nama node, type primitif, pemetaan operator, const-fold binary,
 * dan pembuatan blok/local baru.
 * ============================================================ */

void builderInit(IRBuilder *b, Node *ast, IRModule *module) {
  memset(b, 0, sizeof(*b));
  b->astRef = ast;
  b->module = module;
}

IRValue *cacheOp(IRBuilder *b, int id, IRValue *value) {
  if (id < 0 || !b->astRef || id >= b->astRef->length || !value) return value;
  if (id >= b->opCap) {
    int cap = b->opCap ? b->opCap * 2 : 64;
    while (id >= cap)
      cap *= 2;
    IRValue **grown = gcrealloc(b->ops, sizeof(IRValue *) * cap);
    if (!grown) return value;
    for (int i = b->opLen; i < cap; i++)
      grown[i] = NULL;
    b->ops = grown;
    b->opCap = cap;
  }
  if (id >= b->opLen) b->opLen = id + 1;
  b->ops[id] = value;
  return value;
}

/* ==================== Helpers ==================== */

const char *nodeName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) return a->identifier.name;
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  return NULL;
}

IRType *numberType(void) {
  /* number 64-bit: sizeof = 8 (long long), konsisten analyzer. */
  return irTypeCreate(IR_TYPE_NUMBER, "number", sizeof(long long));
}
IRType *boolType(void) {
  return irTypeCreate(IR_TYPE_BOOLEAN, "boolean", sizeof(int));
}
IRType *nullType(void) {
  return irTypeCreate(IR_TYPE_NULL, "null", 0);
}

IROpcode opcodeFor(const char *op) {
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

int irNumericType(IRValue *v) {
  if (!v || !v->type) return 0;
  return v->type->kind == IR_TYPE_NUMBER || v->type->kind == IR_TYPE_DECIMAL;
}

/* Const-fold `a OP b` saat kedua operand konstanta numerik. */
IRValue *foldBinary(IROpcode op, IRValue *l, IRValue *r) {
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
    case IR_ADD:
      return irNumber(a + b, numberType());
    case IR_SUB:
      return irNumber(a - b, numberType());
    case IR_MUL:
      return irNumber(a * b, numberType());
    case IR_DIV:
      if (b == 0) return NULL;
      return irNumber(a / b, numberType());
    case IR_MOD:
      if (b == 0) return NULL;
      return irNumber(a % b, numberType());
    default:
      break; /* perbandingan tetap lewat jalur double di bawah */
    }
  }
  double a = l->type->kind == IR_TYPE_DECIMAL ? l->data.constant.as.decimal
                                              : (double)l->data.constant.as.number;
  double b = r->type->kind == IR_TYPE_DECIMAL ? r->data.constant.as.decimal
                                              : (double)r->data.constant.as.number;

  double out = 0;
  switch (op) {
  case IR_ADD:
    out = a + b;
    break;
  case IR_SUB:
    out = a - b;
    break;
  case IR_MUL:
    out = a * b;
    break;
  case IR_DIV:
    if (b == 0) return NULL;
    out = a / b;
    break;
  case IR_MOD:
    if (b == 0) return NULL;
    out = (double)((int64_t)a % (int64_t)b);
    break;
  case IR_EQ:
    return irBoolean(a == b, boolType());
  case IR_NE:
    return irBoolean(a != b, boolType());
  case IR_LT:
    return irBoolean(a < b, boolType());
  case IR_LE:
    return irBoolean(a <= b, boolType());
  case IR_GT:
    return irBoolean(a > b, boolType());
  case IR_GE:
    return irBoolean(a >= b, boolType());
  default:
    return NULL;
  }

  return decimal ? irDecimal(out, irTypeCreate(IR_TYPE_DECIMAL, "decimal", sizeof(double)))
                 : irNumber((int64_t)out, numberType());
}

IRValue *emitBinary(IRBuilder *b, IROpcode op, IRValue *l, IRValue *r) {
  IRValue *folded = foldBinary(op, l, r);
  if (folded) return folded;

  IRType *t = irNumericType(l) && irNumericType(r) ? l->type : nullType();
  IRValue *res = irTemp(t);
  irEmit(b->block, irBinary(op, res, l, r));
  return res;
}

IRValue *newLocal(IRBuilder *b, const char *name) {
  IRValue *v = irLocal(nullType(), name);
  scopeBind(b, name, v);
  return v;
}

IRBlock *newBlock(IRBuilder *b, const char *prefix) {
  static char name[64];
  int nextId = b->function->last_block ? (int)b->function->last_block->id + 1 : 0;
  snprintf(name, sizeof(name), "%s.%d", prefix, nextId);
  return irBlockCreate(b->function, name);
}

/* Slot dibaca langsung: mesin eksekusi men-resolve IR_VALUE_LOCAL
 * berdasarkan nama saat load, jadi LOAD eksplisit tidak wajib. */
IRValue *slotValue(IRValue *slot) {
  return slot;
}

/* Nama tipe annotation (NODE_ARRAY_TYPE aware) sebagai string. */
const char *annotationTypeName(IRBuilder *b, int typeId) {
  static char buffer[256];
  if (!formatAstTypeName(b->astRef, typeId, buffer, sizeof(buffer))) return NULL;
  return buffer;
}

/* Node value adalah panggilan `new Contract(...)`? (C-blok design
 * new_memory.txt — alokasi type-driven dari anotasi.) */
bool memoryContractIsContract(Node *node, int valueId) {
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
