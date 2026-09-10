#include <rupa.h>

static bool numeric(RuntimeValue value) {
  return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL;
}
static double numberOf(RuntimeValue value) {
  return value.type == VALUE_DECIMAL ? value.as.decimal : value.as.number;
}
static RuntimeValue numericResult(RuntimeValue left, RuntimeValue right,
                                  double value) {
  return left.type == VALUE_NUMBER && right.type == VALUE_NUMBER
             ? valueNumber((int)value)
             : valueDecimal(value);
}
static char *textOf(RuntimeValue value);

/* Helper: append text to a growing buffer */
static void bufAppend(char **buf, size_t *len, size_t *cap, const char *text) {
  size_t tlen = strlen(text);
  while (*len + tlen + 1 > *cap) {
    *cap = (*cap) ? (*cap) * 2 : 128;
    *buf = realloc(*buf, *cap);
  }
  memcpy(*buf + *len, text, tlen);
  *len += tlen;
  (*buf)[*len] = '\0';
}

/* Helper: append a RuntimeValue as text */
static void bufAppendVal(char **buf, size_t *len, size_t *cap,
                         RuntimeValue val) {
  char *s = textOf(val);
  bufAppend(buf, len, cap, s);
  free(s);
}

static char *textOf(RuntimeValue value) {
  char buffer[64];
  switch (value.type) {
  case VALUE_STRING:
    return strdup(value.as.string ? value.as.string : "");
  case VALUE_NUMBER:
    snprintf(buffer, sizeof(buffer), "%d", value.as.number);
    return strdup(buffer);
  case VALUE_DECIMAL:
    snprintf(buffer, sizeof(buffer), "%g", value.as.decimal);
    return strdup(buffer);
  case VALUE_BOOLEAN:
    return strdup(value.as.boolean ? "true" : "false");
  case VALUE_NULL:
    return strdup("null");
  case VALUE_OBJECT: {
    size_t len = 0, cap = 128;
    char *buf = calloc(cap, 1);
    bufAppend(&buf, &len, &cap, "{");
    bool first = true;
    for (struct RuntimeObjectEntry *e = value.as.object.entries; e;
         e = e->next) {
      if (!first) bufAppend(&buf, &len, &cap, ", ");
      first = false;
      bufAppend(&buf, &len, &cap, e->key ? e->key : "null");
      bufAppend(&buf, &len, &cap, ": ");
      bufAppendVal(&buf, &len, &cap, e->value);
    }
    bufAppend(&buf, &len, &cap, "}");
    return buf;
  }
  case VALUE_ARRAY: {
    size_t len = 0, cap = 128;
    char *buf = calloc(cap, 1);
    bufAppend(&buf, &len, &cap, "[");
    for (int i = 0; i < value.as.array.length; i++) {
      if (i > 0) bufAppend(&buf, &len, &cap, ", ");
      bufAppendVal(&buf, &len, &cap, value.as.array.items[i]);
    }
    bufAppend(&buf, &len, &cap, "]");
    return buf;
  }
  case VALUE_FUNCTION:
    return strdup("<function>");
  case VALUE_NATIVE_FUNCTION:
    return strdup("<native>");
  default:
    return strdup("");
  }
}

InterpreterResult interpretBinary(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  InterpreterResult leftResult =
      interpretExpression(node, ast->binary.left, env, error);
  if (leftResult.flow != FLOW_NORMAL)
    return leftResult;
  InterpreterResult rightResult =
      interpretExpression(node, ast->binary.right, env, error);
  if (rightResult.flow != FLOW_NORMAL)
    return rightResult;
  RuntimeValue left = leftResult.value, right = rightResult.value;
  const char *op = ast->binary.op ? ast->binary.op : "";

  if (!strcmp(op, "+")) {
    if (numeric(left) && numeric(right))
      return resultNormal(
          numericResult(left, right, numberOf(left) + numberOf(right)));
    char *a = textOf(left), *b = textOf(right);
    size_t size = strlen(a) + strlen(b) + 1;
    char *joined = malloc(size);
    if (!joined) {
      free(a);
      free(b);
      return resultNormal(valueNull());
    }
    snprintf(joined, size, "%s%s", a, b);
    free(a);
    free(b);
    return resultNormal(valueString(joined));
  }
  if (numeric(left) && numeric(right)) {
    double a = numberOf(left), b = numberOf(right);
    if (!strcmp(op, "-"))
      return resultNormal(numericResult(left, right, a - b));
    if (!strcmp(op, "*"))
      return resultNormal(numericResult(left, right, a * b));
    if (!strcmp(op, "/"))
      return resultNormal(b ? numericResult(left, right, a / b) : valueNull());
    if (!strcmp(op, "%"))
      return resultNormal(b ? numericResult(left, right, fmod(a, b))
                            : valueNull());
    if (!strcmp(op, "<"))
      return resultNormal(valueBoolean(a < b));
    if (!strcmp(op, ">"))
      return resultNormal(valueBoolean(a > b));
    if (!strcmp(op, "<="))
      return resultNormal(valueBoolean(a <= b));
    if (!strcmp(op, ">="))
      return resultNormal(valueBoolean(a >= b));
  }
  if (!strcmp(op, "=="))
    return resultNormal(valueBoolean(valueEquals(left, right)));
  if (!strcmp(op, "!="))
    return resultNormal(valueBoolean(!valueEquals(left, right)));
  return resultNormal(valueNull());
}
