#pragma once

#if defined(RUPA_PACKAGE_H)

struct RuntimeArray {
  RuntimeValue *items;
  int length;
};

/* RuntimeObjectEntry defined after RuntimeValue to avoid incomplete type. */
struct RuntimeObjectEntry;

struct RuntimeObject {
  struct RuntimeObjectEntry *entries;
};

/* Native function pointer type */
typedef InterpreterResult (*NativeFn)(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);

struct RuntimeNativeFunction {
  const char *name;
  NativeFn func;
  int paramCount;
};

struct RuntimeValue {
  ValueType type;
  union {
    int number;
    double decimal;
    bool boolean;
    char *string;
    struct RuntimeArray array;
    RuntimeFunction *function;
    struct RuntimeNativeFunction *nativeFunc;
    struct RuntimeObject object;
  } as;
};

struct RuntimeObjectEntry {
  char *key;
  RuntimeValue value;
  struct RuntimeObjectEntry *next;
};

#endif
