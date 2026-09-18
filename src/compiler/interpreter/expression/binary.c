#include <rupa.h>
#include <math.h>

static bool numeric(RuntimeValue value) {
  return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL;
}
static double numberOf(RuntimeValue value) {
  return value.type == VALUE_DECIMAL ? value.as.decimal : (double)value.as.number;
}
static RuntimeValue numericResult(RuntimeValue left, RuntimeValue right,
                                  double value) {
  /* number 64-bit: keduanya VALUE_NUMBER -> kembalikan number,
   * selama hasilnya masih mewakili integer di range long long. */
  if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER) {
    if (isfinite(value) && floor(value) == value &&
        value >= -(double)LLONG_MAX && value <= (double)LLONG_MAX)
      return valueNumber((long long)value);
    return valueDecimal(value);
  }
  return valueDecimal(value);
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

/* Wrapper publik textOf untuk mesin IR (lib/compiler/interpreter/text.h). */
char *valueTextOf(RuntimeValue value) { return textOf(value); }

/* Terapkan operator aritmetika/konkatenasi pada dua value — semantik persis
 * interpretBinary, dipakai interpretUpdate untuk compound assignment
 * (+=, -=, *=, /=, %=). *ok=false bila operator tidak berlaku pada tipe. */
RuntimeValue valueBinaryApply(const char *op, RuntimeValue left,
                              RuntimeValue right, bool *ok) {
  *ok = true;
  if (!strcmp(op, "+")) {
    if (numeric(left) && numeric(right))
      return numericResult(left, right, numberOf(left) + numberOf(right));
    char *a = textOf(left), *b = textOf(right);
    size_t size = strlen(a) + strlen(b) + 1;
    char *joined = malloc(size);
    if (!joined) {
      free(a);
      free(b);
      *ok = false;
      return valueNull();
    }
    snprintf(joined, size, "%s%s", a, b);
    free(a);
    free(b);
    RuntimeValue result = valueString(joined);
    free(joined);
    return result;
  }
  if (numeric(left) && numeric(right)) {
    double a = numberOf(left), b = numberOf(right);
    if (!strcmp(op, "-"))
      return numericResult(left, right, a - b);
    if (!strcmp(op, "*"))
      return numericResult(left, right, a * b);
    if (!strcmp(op, "/"))
      return b ? numericResult(left, right, a / b) : valueNull();
    if (!strcmp(op, "%"))
      return b ? numericResult(left, right, fmod(a, b)) : valueNull();
  }
  *ok = false;
  return valueNull();
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
    snprintf(buffer, sizeof(buffer), "%lld", value.as.number);
    return strdup(buffer);
  case VALUE_DECIMAL:
    snprintf(buffer, sizeof(buffer), "%g", value.as.decimal);
    return strdup(buffer);
  case VALUE_BOOLEAN:
    return strdup(value.as.boolean ? "true" : "false");
  case VALUE_NULL:
    return strdup("null");
  case VALUE_PTR:
    return strdup(value.as.ptr ? "<ptr>" : "null");
  case VALUE_OBJECT: {
    size_t len = 0, cap = 128;
    char *buf = calloc(cap, 1);
    bufAppend(&buf, &len, &cap, "{");
    bool first = true;
    for (struct RuntimeObjectEntry *e = value.as.object.entries; e;
         e = e->next) {
      /* Skip anchor implisit (key "") — metadata internal list. */
      if (e->key && e->key[0] == '\0') continue;
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
  RuntimeValue left = leftResult.value;
  const char *op = ast->binary.op ? ast->binary.op : "";

  /* && dan || short-circuit: operand kanan hanya dievaluasi bila diperlukan */
  /* Unary NOT prefix: `!expr` disimpan sebagai binary dengan right == -1 */
  if (!strcmp(op, "!") && ast->binary.right < 0) {
    return resultNormal(valueBoolean(!valueTruthy(left)));
  }

  if (!strcmp(op, "&&") || !strcmp(op, "||")) {
    bool lt = valueTruthy(left);
    if (!strcmp(op, "&&") && !lt)
      return resultNormal(valueBoolean(false));
    if (!strcmp(op, "||") && lt)
      return resultNormal(valueBoolean(true));
    InterpreterResult rightResult =
        interpretExpression(node, ast->binary.right, env, error);
    if (rightResult.flow != FLOW_NORMAL)
      return rightResult;
    return resultNormal(valueBoolean(valueTruthy(rightResult.value)));
  }

  InterpreterResult rightResult =
      interpretExpression(node, ast->binary.right, env, error);
  if (rightResult.flow != FLOW_NORMAL)
    return rightResult;
  RuntimeValue right = rightResult.value;

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
    RuntimeValue result = valueString(joined);
    free(joined);
    return resultNormal(result);
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

/* Format type annotation node menjadi nama tipe string — dipakai analyzer
 * (registry struct) & statement.c. Duplikat logis formatType() statement.c
 * disatukan di sini sebagai versi publik. */
bool formatAstTypeName(Node *node, int typeId, char *buffer, size_t capacity) {
  if (!node || typeId < 0 || typeId >= node->length || !buffer || capacity == 0)
    return false;

  AstNode *type = &node->ast[typeId];
  if (type->type == NODE_IDENTIFIER) {
    int written = snprintf(buffer, capacity, "%s", type->identifier.name);
    return written > 0 && (size_t)written < capacity;
  }
  if (type->type == NODE_LITERAL_ID) {
    int written = snprintf(buffer, capacity, "%s", type->string.value);
    return written > 0 && (size_t)written < capacity;
  }
  if (type->type != NODE_ARRAY_TYPE) return false;

  char element[256];
  if (!formatAstTypeName(node, type->arrayType.elementType, element, sizeof(element)))
    return false;
  int written = snprintf(buffer, capacity, "%s[]", element);
  return written > 0 && (size_t)written < capacity;
}
