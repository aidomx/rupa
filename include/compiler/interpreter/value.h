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
  bool hasReceiver;
  struct RuntimeValue *receiver;
  /* Write-back (mis. o.set({a:1}) pada object kosong): nama binding
   * receiver di env asal + env itu sendiri. Setelah call, interpretCall
   * menulis receiver hasil kembali ke binding ini — receiver object
   * disalin by value saat bind, anchor field pertama tidak terlihat
   * binding asal tanpa write-back. */
  char *bindingName;
  void *bindingEnv;
};

struct RuntimeValue {
  ValueType type;
  union {
    /* number 64-bit (design/rupa_types_const_void_bigint.txt poin 2):
     * VALUE_NUMBER memakai long long — tanpa tipe baru, tanpa aturan
     * promosi, semua jalur (interpreter + IR) otomatis 64-bit. */
    long long number;
    double decimal;
    bool boolean;
    char *string;
    struct RuntimeArray array;
    RuntimeFunction *function;
    struct RuntimeNativeFunction *nativeFunc;
    struct RuntimeObject object;
    /* Handle memori (pin family) — opaque tanpa view type. */
    void *ptr;
  } as;
};

struct RuntimeObjectEntry {
  char *key;
  RuntimeValue value;
  struct RuntimeObjectEntry *next;
};

#endif
