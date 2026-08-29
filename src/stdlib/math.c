#include <rupa.h>

/* ==================== Validation helpers ==================== */
static InterpreterResult mathTypeError(Error *error, const char *name,
                                       const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"TypeError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  (void)name;
  return resultFlow(FLOW_ERROR, valueNull());
}

static bool getNumber(int argc, RuntimeValue *argv, double *out) {
  if (argc < 1 || !argv || !out)
    return false;

  if (argv[0].type == VALUE_NUMBER) {
    *out = (double)argv[0].as.number;
    return true;
  }

  if (argv[0].type == VALUE_DECIMAL) {
    *out = argv[0].as.decimal;
    return true;
  }

  return false;
}

static RuntimeValue numberResult(double value) {
  if (isfinite(value) && floor(value) == value && value >= (double)INT_MIN &&
      value <= (double)INT_MAX)
    return valueNumber((int)value);
  return valueDecimal(value);
}

/* ==================== math.abs(value) ==================== */
static InterpreterResult mathAbs(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "abs", "math.abs() expects a number");
  return resultNormal(numberResult(fabs(value)));
}

/* ==================== math.sqrt(value) ==================== */
static InterpreterResult mathSqrt(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "sqrt", "math.sqrt() expects a number");
  if (value < 0)
    return mathTypeError(error, "sqrt",
                         "math.sqrt() expects a non-negative number");
  return resultNormal(numberResult(sqrt(value)));
}

/* ==================== math.pow(base, exponent) ==================== */
static InterpreterResult mathPow(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  double base, exponent;
  (void)env;
  if (argc < 2 || !getNumber(1, argv, &base) ||
      !getNumber(1, argv + 1, &exponent))
    return mathTypeError(error, "pow", "math.pow() expects two numbers");
  return resultNormal(numberResult(pow(base, exponent)));
}

/* ==================== math.floor(value) ==================== */
static InterpreterResult mathFloor(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "floor", "math.floor() expects a number");
  return resultNormal(valueNumber((int)floor(value)));
}

/* ==================== math.ceil(value) ==================== */
static InterpreterResult mathCeil(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "ceil", "math.ceil() expects a number");
  return resultNormal(valueNumber((int)ceil(value)));
}

/* ==================== math.round(value) ==================== */
static InterpreterResult mathRound(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "round", "math.round() expects a number");
  return resultNormal(valueNumber((int)round(value)));
}

/* ==================== Trigonometric functions ==================== */
static InterpreterResult mathSin(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "sin", "math.sin() expects a number");
  return resultNormal(numberResult(sin(value)));
}

static InterpreterResult mathCos(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "cos", "math.cos() expects a number");
  return resultNormal(numberResult(cos(value)));
}

static InterpreterResult mathTan(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  double value;
  (void)env;
  if (!getNumber(argc, argv, &value))
    return mathTypeError(error, "tan", "math.tan() expects a number");
  return resultNormal(numberResult(tan(value)));
}

/* ==================== Module init ==================== */
static void addEntry(struct RuntimeObjectEntry **head, const char *name,
                     NativeFn fn, int paramCount) {
  struct RuntimeObjectEntry *entry = calloc(1, sizeof(*entry));
  if (!entry)
    return;
  entry->key = strdup(name);
  entry->value = valueNativeFunction(name, fn, paramCount);
  entry->next = *head;
  *head = entry;
}

InterpreterResult stdMathInit(Node *node, int id, RuntimeEnv *env,
                              Error *error) {
  struct RuntimeObjectEntry *entries = NULL;
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  addEntry(&entries, "abs", mathAbs, 1);
  addEntry(&entries, "sqrt", mathSqrt, 1);
  addEntry(&entries, "pow", mathPow, 2);
  addEntry(&entries, "floor", mathFloor, 1);
  addEntry(&entries, "ceil", mathCeil, 1);
  addEntry(&entries, "round", mathRound, 1);
  addEntry(&entries, "sin", mathSin, 1);
  addEntry(&entries, "cos", mathCos, 1);
  addEntry(&entries, "tan", mathTan, 1);

  return resultNormal(valueObject(entries));
}
