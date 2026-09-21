#include "rupa.h"

static uint32_t nextValueId;
static uint32_t nextBlockId;
static uint32_t nextFunctionId;

static char *irDup(const char *s) {
  if (!s) return NULL;
  return gcdup(s);
}

static IRInstruction *newInstruction(IROpcode op) {
  IRInstruction *instruction = gccalloc(1, sizeof(*instruction));
  if (instruction) instruction->op = op;
  return instruction;
}

IRModule *createIR(void) {
  return gccalloc(1, sizeof(IRModule));
}

IRType *irTypeCreate(IRTypeKind kind, const char *name, size_t size) {
  IRType *type = gccalloc(1, sizeof(*type));
  if (!type) return NULL;

  type->kind = kind;
  type->name = irDup(name);
  type->size = size;
  return type;
}

IRType *irTypeArray(IRType *element) {
  IRType *type = irTypeCreate(IR_TYPE_ARRAY, NULL, 0);
  if (type) type->element = element;
  return type;
}

void irModuleAddType(IRModule *module, IRType *type) {
  if (!module || !type) return;

  IRType **types = gcrealloc(module->types, sizeof(*types) * (module->type_count + 1));
  if (!types) return;

  module->types = types;
  module->types[module->type_count++] = type;
}

IRValue *irTemp(IRType *type) {
  IRValue *value = gccalloc(1, sizeof(*value));
  if (!value) return NULL;
  value->kind = IR_VALUE_TEMP;
  value->type = type;
  value->id = nextValueId++;
  return value;
}

static IRValue *namedValue(IRValueKind kind, IRType *type, const char *name) {
  IRValue *value = gccalloc(1, sizeof(*value));
  if (!value) return NULL;

  value->kind = kind;
  value->type = type;
  value->id = nextValueId++;
  value->data.name = irDup(name);
  return value;
}

IRValue *irParam(IRType *type, const char *name) {
  return namedValue(IR_VALUE_PARAM, type, name);
}

IRValue *irLocal(IRType *type, const char *name) {
  return namedValue(IR_VALUE_LOCAL, type, name);
}

IRValue *irGlobal(IRType *type, const char *name) {
  return namedValue(IR_VALUE_GLOBAL, type, name);
}

IRValue *irFunctionValue(IRType *type, const char *name) {
  return namedValue(IR_VALUE_FUNCTION, type, name);
}

static IRValue *constantValue(IRConstantKind kind, IRType *type) {
  IRValue *value = gccalloc(1, sizeof(*value));
  if (!value) return NULL;
  value->kind = IR_VALUE_CONSTANT;
  value->type = type;
  value->id = nextValueId++;
  value->data.constant.kind = kind;
  return value;
}

IRValue *irNumber(int64_t number, IRType *type) {
  IRValue *value = constantValue(IR_CONST_NUMBER, type);
  if (value) value->data.constant.as.number = number;
  return value;
}

IRValue *irDecimal(double decimal, IRType *type) {
  IRValue *value = constantValue(IR_CONST_DECIMAL, type);
  if (value) value->data.constant.as.decimal = decimal;
  return value;
}

IRValue *irBoolean(int boolean, IRType *type) {
  IRValue *value = constantValue(IR_CONST_BOOLEAN, type);
  if (value) value->data.constant.as.boolean = boolean != 0;
  return value;
}

IRValue *irString(const char *string, IRType *type) {
  IRValue *value = constantValue(IR_CONST_STRING, type);
  if (!value) return NULL;
  value->data.constant.as.string = irDup(string);
  return value;
}

IRValue *irNull(IRType *type) {
  return constantValue(IR_CONST_NULL, type);
}

void irValueFree(IRValue *value) {
  if (!value) return;

  if (value->kind == IR_VALUE_CONSTANT && value->data.constant.kind == IR_CONST_STRING)
    gcfree(value->data.constant.as.string);
  else if (value->kind != IR_VALUE_TEMP && value->kind != IR_VALUE_CONSTANT)
    gcfree(value->data.name);

  gcfree(value);
}

IRFunction *irFunctionCreate(IRModule *module, const char *name, IRType *return_type) {
  if (!module) return NULL;

  IRFunction *function = gccalloc(1, sizeof(*function));
  if (!function) return NULL;

  function->id = nextFunctionId++;
  function->name = irDup(name);
  function->return_type = return_type;

  if (!module->first_function)
    module->first_function = function;
  else
    module->last_function->next = function;
  module->last_function = function;

  return function;
}

IRBlock *irBlockCreate(IRFunction *function, const char *name) {
  if (!function) return NULL;

  IRBlock *block = gccalloc(1, sizeof(*block));
  if (!block) return NULL;

  block->id = nextBlockId++;
  block->name = irDup(name);

  if (!function->first_block)
    function->first_block = block;
  else
    function->last_block->next = block;
  function->last_block = block;

  if (!function->current_block) function->current_block = block;

  return block;
}

void irSetBlock(IRFunction *function, IRBlock *block) {
  if (function) function->current_block = block;
}

void irEmit(IRBlock *block, IRInstruction *instruction) {
  if (!block || !instruction) return;

  if (!block->first)
    block->first = instruction;
  else
    block->last->next = instruction;
  block->last = instruction;
}

IRInstruction *irConst(IRValue *result, IRValue *value) {
  IRInstruction *i = newInstruction(IR_CONST);
  if (!i) return NULL;
  i->result = result;
  i->data.unary.value = value;
  return i;
}

IRInstruction *irLoad(IRValue *result, IRValue *source) {
  IRInstruction *i = newInstruction(IR_LOAD);
  if (!i) return NULL;
  i->result = result;
  i->data.unary.value = source;
  return i;
}

IRInstruction *irStore(IRValue *target, IRValue *value) {
  return irStoreAt(target, value, false, -1);
}

/* Store dengan metadata deklarasi: isConst = slot dikunci setelah write
 * pertama (executor menolak store berikutnya via ConstError); nodeId =
 * lokasi AST untuk error reassignment. */
IRInstruction *irStoreAt(IRValue *target, IRValue *value, bool isConst, int nodeId) {
  IRInstruction *i = newInstruction(IR_STORE);
  if (!i) return NULL;
  i->data.store.target = target;
  i->data.store.value = value;
  i->data.store.isConst = isConst;
  i->data.store.nodeId = nodeId;
  return i;
}

IRInstruction *irBinary(IROpcode op, IRValue *result, IRValue *left, IRValue *right) {
  IRInstruction *i = newInstruction(op);
  if (!i) return NULL;
  i->result = result;
  i->data.binary.left = left;
  i->data.binary.right = right;
  return i;
}

IRInstruction *irUnary(IROpcode op, IRValue *result, IRValue *value) {
  IRInstruction *i = newInstruction(op);
  if (!i) return NULL;
  i->result = result;
  i->data.unary.value = value;
  return i;
}

IRInstruction *irMemberGet(IRValue *result, IRValue *object, const char *member) {
  IRInstruction *i = newInstruction(IR_MEMBER_GET);
  if (!i) return NULL;
  i->result = result;
  i->data.member_get.object = object;
  i->data.member_get.member = irDup(member);
  return i;
}

IRInstruction *irMemberSet(IRValue *object, const char *member, IRValue *value) {
  IRInstruction *i = newInstruction(IR_MEMBER_SET);
  if (!i) return NULL;
  i->data.member_set.object = object;
  i->data.member_set.member = irDup(member);
  i->data.member_set.value = value;
  return i;
}

IRInstruction *irIndexGet(IRValue *result, IRValue *array, IRValue *index) {
  IRInstruction *i = newInstruction(IR_INDEX_GET);
  if (!i) return NULL;
  i->result = result;
  i->data.index_get.array = array;
  i->data.index_get.index = index;
  return i;
}

IRInstruction *irIndexSet(IRValue *array, IRValue *index, IRValue *value) {
  IRInstruction *i = newInstruction(IR_INDEX_SET);
  if (!i) return NULL;
  i->data.index_set.array = array;
  i->data.index_set.index = index;
  i->data.index_set.value = value;
  return i;
}

IRInstruction *irCall(IRValue *result, IRValue *callee, IRValue **args, size_t count) {
  IRInstruction *i = newInstruction(IR_CALL);
  if (!i) return NULL;
  i->result = result;
  i->data.call.callee = callee;
  i->data.call.args = args;
  i->data.call.count = count;
  return i;
}

IRInstruction *irInterp(int nodeId, IRType *resultType) {
  /* Payload dibawa lewat call.count (node id AST); tanpa callee/args.
   * Trampoline ke interpretNode untuk node yang butuh runtime penuh. */
  IRInstruction *i = newInstruction(IR_INTERP);
  if (!i) return NULL;
  i->result = resultType ? irTemp(resultType) : NULL;
  i->data.call.count = (size_t)nodeId;
  return i;
}

IRInstruction *irReturn(IRValue *value) {
  IRInstruction *i = newInstruction(IR_RETURN);
  if (i) i->data.return_value.value = value;
  return i;
}

IRInstruction *irJump(IRBlock *target) {
  IRInstruction *i = newInstruction(IR_JUMP);
  if (i) i->data.jump.target = target;
  return i;
}

IRInstruction *irBranch(IRValue *condition, IRBlock *then_block, IRBlock *else_block) {
  IRInstruction *i = newInstruction(IR_BRANCH);
  if (!i) return NULL;
  i->data.branch.condition = condition;
  i->data.branch.then_block = then_block;
  i->data.branch.else_block = else_block;
  return i;
}

IRInstruction *irAlloc(IRValue *result, IRType *type, IRValue *count, int zeroed) {
  IRInstruction *i = newInstruction(IR_ALLOC);
  if (!i) return NULL;
  i->result = result;
  i->data.alloc.type = type;
  i->data.alloc.count = count;
  i->data.alloc.zeroed = zeroed != 0;
  return i;
}

IRInstruction *irRealloc(IRValue *result, IRValue *pointer, IRValue *size) {
  IRInstruction *i = newInstruction(IR_REALLOC);
  if (!i) return NULL;
  i->result = result;
  i->data.realloc.pointer = pointer;
  i->data.realloc.size = size;
  return i;
}

IRInstruction *irFree(IRValue *pointer) {
  IRInstruction *i = newInstruction(IR_FREE);
  if (i) i->data.free.pointer = pointer;
  return i;
}

IRInstruction *irStrSlotGet(IRValue *result, IRValue *pointer) {
  IRInstruction *i = newInstruction(IR_STRSLOT_GET);
  if (!i) return NULL;
  i->result = result;
  i->data.strslot_get.pointer = pointer;
  return i;
}

IRInstruction *irStrSlotSet(IRValue *pointer, IRValue *value) {
  IRInstruction *i = newInstruction(IR_STRSLOT_SET);
  if (!i) return NULL;
  i->data.strslot_set.pointer = pointer;
  i->data.strslot_set.value = value;
  return i;
}

IRInstruction *irCast(IRValue *result, IRValue *value, IRType *type) {
  IRInstruction *i = newInstruction(IR_CAST);
  if (!i) return NULL;
  i->result = result;
  i->data.cast.value = value;
  i->data.cast.type = type;
  return i;
}

void irInstructionFree(IRInstruction *instruction) {
  if (!instruction) return;

  switch (instruction->op) {
  case IR_MEMBER_GET:
    gcfree(instruction->data.member_get.member);
    break;
  case IR_MEMBER_SET:
    gcfree(instruction->data.member_set.member);
    break;
  case IR_CALL:
    gcfree(instruction->data.call.args);
    break;
  default:
    break;
  }

  gcfree(instruction);
}

void irModuleFree(IRModule *module) {
  if (!module) return;

  for (IRFunction *function = module->first_function; function;) {
    IRFunction *next_function = function->next;

    gcfree(function->params);

    for (IRBlock *block = function->first_block; block;) {
      IRBlock *next_block = block->next;

      for (IRInstruction *i = block->first; i;) {
        IRInstruction *next = i->next;
        irInstructionFree(i);
        i = next;
      }

      gcfree(block->name);
      gcfree(block);
      block = next_block;
    }

    gcfree(function->name);
    gcfree(function);
    function = next_function;
  }

  for (size_t i = 0; i < module->type_count; i++) {
    gcfree(module->types[i]->name);
    gcfree(module->types[i]);
  }

  gcfree(module->types);
  gcfree(module);
}

/* Semantic check: validasi value terhadap nama tipe saat eksekusi IR.
 * Nama tipe dibawa sebagai string (gcstrdup, dikelola GC).
 * nodeId = AST id assignment/annotation (-1 jika tidak ada) untuk view
 * type check pin family (provenance sizeof di sisi kanan). */
IRInstruction *irCheckAt(IRValue *value, const char *type, int nodeId) {
  IRInstruction *i = newInstruction(IR_CHECK);
  if (!i) return NULL;
  i->result = NULL;
  i->data.check.value = value;
  i->data.check.type = type ? gcstrdup(type) : NULL;
  i->data.check.nodeId = nodeId;
  return i;
}

IRInstruction *irCheck(IRValue *value, const char *type) {
  return irCheckAt(value, type, -1);
}

/* Trampoline IR_CHECK ke interpretNode: node annotation/assignment
 * bertype dievaluasi penuh oleh interpreter (validasi struct-first
 * terjadi di interpretStatement). Payload call.count = AST node id. */
IRInstruction *irInterpCheck(int nodeId) {
  return irInterp(nodeId, NULL);
}
