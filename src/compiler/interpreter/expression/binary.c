#include <rupa.h>

static bool numeric(RuntimeValue value) { return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL; }
static double numberOf(RuntimeValue value) { return value.type == VALUE_DECIMAL ? value.as.decimal : value.as.number; }
static RuntimeValue numericResult(RuntimeValue left, RuntimeValue right, double value) {
  return left.type == VALUE_NUMBER && right.type == VALUE_NUMBER ? valueNumber((int)value) : valueDecimal(value);
}
static char *textOf(RuntimeValue value) {
  char buffer[64];
  switch (value.type) {
  case VALUE_STRING: return strdup(value.as.string ? value.as.string : "");
  case VALUE_NUMBER: snprintf(buffer, sizeof(buffer), "%d", value.as.number); break;
  case VALUE_DECIMAL: snprintf(buffer, sizeof(buffer), "%g", value.as.decimal); break;
  case VALUE_BOOLEAN: return strdup(value.as.boolean ? "true" : "false");
  case VALUE_NULL: return strdup("null");
  default: return strdup("");
  }
  return strdup(buffer);
}

InterpreterResult interpretBinary(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  InterpreterResult leftResult = interpretExpression(node, ast->binary.left, env, error);
  if (leftResult.flow != FLOW_NORMAL) return leftResult;
  InterpreterResult rightResult = interpretExpression(node, ast->binary.right, env, error);
  if (rightResult.flow != FLOW_NORMAL) return rightResult;
  RuntimeValue left = leftResult.value, right = rightResult.value;
  const char *op = ast->binary.op ? ast->binary.op : "";

  if (!strcmp(op, "+")) {
    if (numeric(left) && numeric(right)) return resultNormal(numericResult(left, right, numberOf(left) + numberOf(right)));
    char *a = textOf(left), *b = textOf(right);
    size_t size = strlen(a) + strlen(b) + 1;
    char *joined = malloc(size);
    if (!joined) { free(a); free(b); return resultNormal(valueNull()); }
    snprintf(joined, size, "%s%s", a, b);
    free(a); free(b);
    return resultNormal(valueString(joined));
  }
  if (numeric(left) && numeric(right)) {
    double a = numberOf(left), b = numberOf(right);
    if (!strcmp(op, "-")) return resultNormal(numericResult(left, right, a - b));
    if (!strcmp(op, "*")) return resultNormal(numericResult(left, right, a * b));
    if (!strcmp(op, "/")) return resultNormal(b ? numericResult(left, right, a / b) : valueNull());
    if (!strcmp(op, "<")) return resultNormal(valueBoolean(a < b));
    if (!strcmp(op, ">")) return resultNormal(valueBoolean(a > b));
    if (!strcmp(op, "<=")) return resultNormal(valueBoolean(a <= b));
    if (!strcmp(op, ">=")) return resultNormal(valueBoolean(a >= b));
  }
  if (!strcmp(op, "==")) return resultNormal(valueBoolean(valueEquals(left, right)));
  if (!strcmp(op, "!=")) return resultNormal(valueBoolean(!valueEquals(left, right)));
  return resultNormal(valueNull());
}
