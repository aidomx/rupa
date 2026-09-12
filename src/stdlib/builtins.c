#include <rupa.h>

/* ==================== type(value) ==================== */
/* Returns the type name of a value as a string */

static InterpreterResult builtinType(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return resultNormal(valueString("undefined"));

  const char *typeName;
  switch (argv[0].type) {
  case VALUE_NULL:
    typeName = "null";
    break;
  case VALUE_NUMBER:
    typeName = "number";
    break;
  case VALUE_DECIMAL:
    typeName = "decimal";
    break;
  case VALUE_BOOLEAN:
    typeName = "boolean";
    break;
  case VALUE_STRING:
    typeName = "string";
    break;
  case VALUE_ARRAY:
    typeName = "array";
    break;
  case VALUE_OBJECT:
    typeName = "object";
    break;
  case VALUE_FUNCTION:
    typeName = "function";
    break;
  case VALUE_NATIVE_FUNCTION:
    typeName = "native";
    break;
  default:
    typeName = "unknown";
    break;
  }

  return resultNormal(valueString(gcstrdup(typeName)));
}

/* ==================== len(value) ==================== */
/* Returns length of string, array, or object */

static InterpreterResult builtinLen(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return resultFlow(FLOW_ERROR, valueNull());

  switch (argv[0].type) {
  case VALUE_STRING:
    if (argv[0].as.string) return resultNormal(valueNumber((int)strlen(argv[0].as.string)));
    return resultNormal(valueNumber(0));
  case VALUE_ARRAY:
    return resultNormal(valueNumber(argv[0].as.array.length));
  case VALUE_OBJECT: {
    int count = 0;
    for (struct RuntimeObjectEntry *e = argv[0].as.object.entries; e; e = e->next)
      count++;
    return resultNormal(valueNumber(count));
  }
  default:
    return resultFlow(FLOW_ERROR, valueNull());
  }
}

/* ==================== isNull(value) ==================== */

static InterpreterResult builtinIsNull(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                       Error *error) {
  (void)env;
  if (argc < 1) return resultNormal(valueBoolean(true));
  return resultNormal(valueBoolean(argv[0].type == VALUE_NULL));
}

/* ==================== toNumber(value) ==================== */

static InterpreterResult builtinToNumber(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                         Error *error) {
  (void)env;
  if (argc < 1) return resultNormal(valueNumber(0));

  switch (argv[0].type) {
  case VALUE_NUMBER:
  case VALUE_DECIMAL:
    return resultNormal(argv[0]);
  case VALUE_STRING:
    if (argv[0].as.string) {
      char *end;
      double val = strtod(argv[0].as.string, &end);
      if (end != argv[0].as.string) return resultNormal(valueNumber((int)val));
    }
    return resultNormal(valueNumber(0));
  case VALUE_BOOLEAN:
    return resultNormal(valueNumber(argv[0].as.boolean ? 1 : 0));
  default:
    return resultNormal(valueNumber(0));
  }
}

/* ==================== toString(value) ==================== */

static InterpreterResult builtinToString(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                         Error *error) {
  (void)env;
  if (argc < 1) return resultNormal(valueString(""));

  /* If already string, return as-is */
  if (argv[0].type == VALUE_STRING) return resultNormal(argv[0]);

  /* Use textOf-like conversion */
  char buf[256];
  switch (argv[0].type) {
  case VALUE_NULL:
    return resultNormal(valueString(gcstrdup("null")));
  case VALUE_NUMBER:
    snprintf(buf, sizeof(buf), "%d", argv[0].as.number);
    return resultNormal(valueString(gcstrdup(buf)));
  case VALUE_DECIMAL:
    snprintf(buf, sizeof(buf), "%g", argv[0].as.decimal);
    return resultNormal(valueString(gcstrdup(buf)));
  case VALUE_BOOLEAN:
    return resultNormal(valueString(gcstrdup(argv[0].as.boolean ? "true" : "false")));
  default:
    return resultNormal(valueString(gcstrdup("[object]")));
  }
}

/* ==================== Register builtins ==================== */

void builtinsInit(RuntimeEnv *env) {
  if (!env) return;

  semSet(env, "type", valueNativeFunction("type", builtinType, 1));
  semSet(env, "len", valueNativeFunction("len", builtinLen, 1));
  semSet(env, "isNull", valueNativeFunction("isNull", builtinIsNull, 1));
  semSet(env, "toNumber", valueNativeFunction("toNumber", builtinToNumber, 1));
  semSet(env, "toString", valueNativeFunction("toString", builtinToString, 1));
}
