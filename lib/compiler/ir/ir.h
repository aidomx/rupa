#pragma once
/* IR is deliberately independent from AST NodeType and RuntimeValue. */

#if defined(RUPA_PACKAGE_H)
struct IRType {
  IRTypeKind kind;
  char *name;
  IRType *element;
  size_t size;
};

struct IRValue {
  IRValueKind kind;
  IRType *type;
  uint32_t id;
  union {
    struct {
      IRConstantKind kind;
      union {
        int64_t number;
        double decimal;
        int boolean;
        char *string;
      } as;
    } constant;
    char *name;
  } data;
};

struct IRInstruction {
  IROpcode op;
  IRValue *result;
  union {
    struct {
      IRValue *value;
    } unary;
    struct {
      IRValue *left;
      IRValue *right;
    } binary;
    struct {
      IRValue *target;
      IRValue *value;
      bool isConst; /* `const x = v` — store deklarasi mengunci slot setelah write */
      int nodeId;   /* AST id assignment — lokasi ConstError */
    } store;
    struct {
      IRValue *object;
      char *member;
    } member_get;
    struct {
      IRValue *object;
      char *member;
      IRValue *value;
    } member_set;
    struct {
      IRValue *array;
      IRValue *index;
    } index_get;
    struct {
      IRValue *array;
      IRValue *index;
      IRValue *value;
    } index_set;
    struct {
      IRValue *callee;
      IRValue **args;
      size_t count;
    } call;
    struct {
      IRValue *value;
    } return_value;
    struct {
      IRBlock *target;
    } jump;
    struct {
      IRValue *condition;
      IRBlock *then_block;
      IRBlock *else_block;
    } branch;
    struct {
      IRType *type;
      IRValue *count;
      int zeroed;
    } alloc;
    struct {
      IRValue *pointer;
      IRValue *size;
    } realloc;
    struct {
      IRValue *pointer;
    } free;
    /* String slot (design/str_memory.txt): baca/tulis char* di dalam
     * handle Contract string — read/write-through pada variable. */
    struct {
      IRValue *pointer;
    } strslot_get;
    struct {
      IRValue *pointer;
      IRValue *value;
    } strslot_set;
    struct {
      IRValue *value;
      IRType *type;
    } cast;
    struct {
      IRValue *value;
      char *type;
      int nodeId; /* AST id assignment/annotation — view check pin family */
    } check;
  } data;
  IRInstruction *next;
};

struct IRBlock {
  uint32_t id;
  char *name;
  IRInstruction *first;
  IRInstruction *last;
  IRBlock *next;
};

struct IRFunction {
  uint32_t id;
  char *name;
  IRType *return_type;

  IRValue **params;
  size_t param_count;

  IRBlock *first_block;
  IRBlock *last_block;
  IRBlock *current_block;

  IRFunction *next;
};

struct IRModule {
  IRType **types;
  size_t type_count;

  IRFunction *first_function;
  IRFunction *last_function;
};

/* module */
IRModule *createIR(void);
void irModuleFree(IRModule *module);
IRType *irTypeCreate(IRTypeKind kind, const char *name, size_t size);
IRType *irTypeArray(IRType *element);
void irModuleAddType(IRModule *module, IRType *type);

/* values */
IRValue *irTemp(IRType *type);
IRValue *irParam(IRType *type, const char *name);
IRValue *irLocal(IRType *type, const char *name);
IRValue *irGlobal(IRType *type, const char *name);
IRValue *irFunctionValue(IRType *type, const char *name);
IRValue *irNumber(int64_t value, IRType *type);
IRValue *irDecimal(double value, IRType *type);
IRValue *irBoolean(int value, IRType *type);
IRValue *irString(const char *value, IRType *type);
IRValue *irNull(IRType *type);
void irValueFree(IRValue *value);

/* functions / blocks */
IRFunction *irFunctionCreate(IRModule *module, const char *name, IRType *return_type);
IRBlock *irBlockCreate(IRFunction *function, const char *name);
void irSetBlock(IRFunction *function, IRBlock *block);
void irEmit(IRBlock *block, IRInstruction *instruction);

/* instructions */
IRInstruction *irConst(IRValue *result, IRValue *value);
IRInstruction *irLoad(IRValue *result, IRValue *source);
IRInstruction *irStore(IRValue *target, IRValue *value);
IRInstruction *irStoreAt(IRValue *target, IRValue *value, bool isConst, int nodeId);
IRInstruction *irBinary(IROpcode op, IRValue *result, IRValue *left, IRValue *right);
IRInstruction *irUnary(IROpcode op, IRValue *result, IRValue *value);
IRInstruction *irMemberGet(IRValue *result, IRValue *object, const char *member);
IRInstruction *irMemberSet(IRValue *object, const char *member, IRValue *value);
IRInstruction *irIndexGet(IRValue *result, IRValue *array, IRValue *index);
IRInstruction *irIndexSet(IRValue *array, IRValue *index, IRValue *value);
IRInstruction *irCall(IRValue *result, IRValue *callee, IRValue **args, size_t count);
IRInstruction *irInterp(int nodeId, IRType *resultType);
IRInstruction *irReturn(IRValue *value);
IRInstruction *irJump(IRBlock *target);
IRInstruction *irBranch(IRValue *condition, IRBlock *then_block, IRBlock *else_block);
IRInstruction *irAlloc(IRValue *result, IRType *type, IRValue *count, int zeroed);
IRInstruction *irRealloc(IRValue *result, IRValue *pointer, IRValue *size);
IRInstruction *irFree(IRValue *pointer);
IRInstruction *irStrSlotGet(IRValue *result, IRValue *pointer);
IRInstruction *irStrSlotSet(IRValue *pointer, IRValue *value);
IRInstruction *irCast(IRValue *result, IRValue *value, IRType *type);
IRInstruction *irCheck(IRValue *value, const char *type);
IRInstruction *irCheckAt(IRValue *value, const char *type, int nodeId);
IRInstruction *irInterpCheck(int nodeId);
void irInstructionFree(IRInstruction *instruction);

#endif
