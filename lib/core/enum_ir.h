#pragma once

enum IRConstantKind {
  IR_CONST_NONE = 0,
  IR_CONST_NULL,
  IR_CONST_NUMBER,
  IR_CONST_DECIMAL,
  IR_CONST_BOOLEAN,
  IR_CONST_STRING,
};

enum IROpcode {
  IR_NOP = 0,

  /* Values */
  IR_CONST,
  IR_LOAD,
  IR_STORE,
  /* Arithmetic */
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,
  IR_MOD,
  IR_NEG,

  /* Comparison */
  IR_EQ,
  IR_NE,
  IR_LT,
  IR_LE,
  IR_GT,
  IR_GE,
  /* Logical */
  IR_NOT,
  IR_AND,
  IR_OR,
  /* Aggregate */
  IR_MEMBER_GET,
  IR_MEMBER_SET,
  IR_INDEX_GET,
  IR_INDEX_SET,
  /* Calls / functions */
  IR_CALL,
  IR_RETURN,
  /* Trampoline: evaluasi AST node via interpreter (module loading,
   * export policy, namespace — runtime di luar cakupan IR murni). */
  IR_INTERP,
  /* Semantic check: validasi value terhadap nama tipe (analyzer,
   * struct-first typing). Error ditempel ke machine->error. */
  IR_CHECK,
  /* Control flow */
  IR_JUMP,
  IR_BRANCH,
  /* Memory */
  IR_ALLOC,
  IR_REALLOC,
  IR_FREE,
  IR_STRSLOT_GET,
  IR_STRSLOT_SET,
  /* Conversion */
  IR_CAST
};

enum IRValueKind {
  IR_VALUE_NONE = 0,
  IR_VALUE_TEMP,
  IR_VALUE_PARAM,
  IR_VALUE_LOCAL,
  IR_VALUE_GLOBAL,
  IR_VALUE_CONSTANT,
  IR_VALUE_FUNCTION
};

enum IRTypeKind {
  IR_TYPE_VOID = 0,
  IR_TYPE_NULL,
  IR_TYPE_BOOLEAN,
  IR_TYPE_NUMBER,
  IR_TYPE_DECIMAL,
  IR_TYPE_STRING,
  IR_TYPE_OBJECT,
  IR_TYPE_ARRAY,
  IR_TYPE_FUNCTION,
  IR_TYPE_POINTER,
  IR_TYPE_STRUCT
};
