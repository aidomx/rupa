#include <rupa.h>

RuntimeValue valueNull(void) { return (RuntimeValue){.type = VALUE_NULL}; }

RuntimeValue valueNumber(int value) {
  return (RuntimeValue){.type = VALUE_NUMBER, .as.number = value};
}

RuntimeValue valueDecimal(double value) {
  return (RuntimeValue){.type = VALUE_DECIMAL, .as.decimal = value};
}

RuntimeValue valueBoolean(bool value) {
  return (RuntimeValue){.type = VALUE_BOOLEAN, .as.boolean = value};
}

RuntimeValue valueString(const char *value) {
  if (!value)
    return (RuntimeValue){.type = VALUE_STRING, .as.string = NULL};

  size_t length = strlen(value);
  size_t begin = 0;
  size_t end = length;
  if (length >= 2 && ((value[0] == '"' && value[length - 1] == '"') ||
                      (value[0] == '\'' && value[length - 1] == '\''))) {
    begin = 1;
    end = length - 1;
  }

  char *text = malloc(end - begin + 1);
  if (!text)
    return (RuntimeValue){.type = VALUE_STRING, .as.string = NULL};

  size_t out = 0;
  for (size_t i = begin; i < end; i++) {
    if (value[i] == '\\' && i + 1 < end) {
      i++;
      switch (value[i]) {
      case 'n':
        text[out++] = '\n';
        continue;
      case 't':
        text[out++] = '\t';
        continue;
      case 'r':
        text[out++] = '\r';
        continue;
      case '\\':
        text[out++] = '\\';
        continue;
      case '"':
        text[out++] = '"';
        continue;
      case '\'':
        text[out++] = '\'';
        continue;
      default:
        text[out++] = value[i];
        continue;
      }
    }
    text[out++] = value[i];
  }
  text[out] = '\0';
  return (RuntimeValue){.type = VALUE_STRING, .as.string = text};
}

RuntimeValue valueArray(RuntimeValue *items, int length) {
  return (RuntimeValue){.type = VALUE_ARRAY,
                        .as.array = {.items = items, .length = length}};
}

/**
 * Evaluate a small Rupa expression snippet (e.g. "add(1,2)" or
 * "users.name") extracted from inside a `{{ ... }}` interpolation block.
 *
 * Uses the CURRENT runtime environment `env` so it can see already
 * declared variables and functions - it builds a throwaway token/AST pool
 * for just this snippet, but that's fine: RuntimeFunction carries its own
 * defining Node pool (see interpretCall()), and variable/function lookup
 * goes through `env` by name, not through which Node pool is asking.
 *
 * Returns true and fills *out on success. Returns false (leaving *out
 * untouched) on lex/parse/eval failure, so the caller can fall back to
 * printing the interpolation block literally instead of crashing.
 */
static bool evalInterpExpr(const char *exprSrc, RuntimeEnv *env,
                           Error *error, RuntimeValue *out) {
  if (!exprSrc || !*exprSrc)
    return false;

  State *state = createGlobalState(8, false);
  if (!state || !state->buffer)
    return false;

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  size_t len = strlen(exprSrc);
  if ((int)len >= buffer->capacity)
    return false;

  memcpy(buffer->value, exprSrc, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  if (!addToHistory(state) || !addToInput(state))
    return false;

  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting))
    return false;

  Request request = createRequest(tokens, 8);
  int end = grammarLineEnd(tokens, 0);
  int exprId = grammarParseExpr(&request, 0, end);
  if (exprId < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "SyntaxError",
                                  .message = "invalid expression inside {{ }}",
                                  .line = 0, .row = 0, .type = ERR_SYNTAX});
    return false;
  }

  InterpreterResult result = interpretNode(request.node, exprId, env, error);
  if (result.flow == FLOW_ERROR)
    return false;

  *out = result.value;
  return true;
}

static void printStringWithInterp(const char *str, RuntimeEnv *env,
                                  Error *error) {
  if (!str)
    return;
  const char *p = str;
  while (*p) {
    /* `{{ expr }}`: full expression - function call, member access, dst. */
    if (p[0] == '{' && p[1] == '{') {
      const char *end = strstr(p + 2, "}}");
      if (end) {
        int len = (int)(end - p - 2);
        if (len > 0) {
          char expr[512];
          if (len >= (int)sizeof(expr))
            len = (int)sizeof(expr) - 1;
          memcpy(expr, p + 2, len);
          expr[len] = '\0';

          RuntimeValue val;
          if (evalInterpExpr(expr, env, error, &val)) {
            valuePrint(val);
          } else {
            printf("{{%s}}", expr); /* gagal parse/eval: tampilkan apa adanya */
          }
        } else {
          printf("{{}}"); /* empty braces */
        }
        p = end + 2;
        continue;
      }
    }
    /* `{ name }`: nama variabel tunggal (lookup langsung, tanpa parser). */
    if (*p == '{') {
      /* Find matching closing brace */
      const char *end = strchr(p + 1, '}');
      if (end) {
        /* Extract variable name */
        int len = (int)(end - p - 1);
        if (len > 0) {
          char name[256];
          if (len >= (int)sizeof(name))
            len = (int)sizeof(name) - 1;
          memcpy(name, p + 1, len);
          name[len] = '\0';
          /* Resolve variable */
          RuntimeValue val;
          if (env && semGet(env, name, &val)) {
            valuePrint(val);
          } else {
            printf("{%s}", name); /* unresolved */
          }
        } else {
          printf("{}"); /* empty braces */
        }
        p = end + 1;
        continue;
      }
    }
    putchar(*p);
    p++;
  }
}

void valuePrintInterp(RuntimeValue value, RuntimeEnv *env, Error *error) {
  if (value.type == VALUE_STRING) {
    printStringWithInterp(value.as.string, env, error);
    return;
  }
  valuePrint(value);
}

void valuePrint(RuntimeValue value) {
  switch (value.type) {
  case VALUE_NUMBER:
    printf("%d", value.as.number);
    break;
  case VALUE_DECIMAL:
    printf("%g", value.as.decimal);
    break;
  case VALUE_BOOLEAN:
    printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VALUE_STRING:
    printf("%s", value.as.string ? value.as.string : "");
    break;
  case VALUE_FUNCTION:
    printf("<function>");
    break;
  case VALUE_ARRAY:
    putchar('[');
    for (int i = 0; i < value.as.array.length; i++) {
      if (i)
        printf(", ");
      valuePrint(value.as.array.items[i]);
    }
    putchar(']');
    break;
  case VALUE_OBJECT: {
    bool first = true;
    putchar('{');
    for (struct RuntimeObjectEntry *e = value.as.object.entries; e;
         e = e->next) {
      /* Skip hidden _private metadata */
      if (e->key && strcmp(e->key, "_private") == 0)
        continue;
      if (!first)
        printf(", ");
      printf("%s: ", e->key ? e->key : "?");
      valuePrint(e->value);
      first = false;
    }
    putchar('}');
    break;
  }
  default:
    printf("undefined");
    break;
  }
}

bool valueTruthy(RuntimeValue value) {
  switch (value.type) {
  case VALUE_NULL:
    return false;
  case VALUE_BOOLEAN:
    return value.as.boolean;
  case VALUE_NUMBER:
    return value.as.number != 0;
  case VALUE_DECIMAL:
    return value.as.decimal != 0;
  case VALUE_STRING:
    return value.as.string && value.as.string[0];
  default:
    return true;
  }
}

bool valueEquals(RuntimeValue left, RuntimeValue right) {
  if (left.type != right.type)
    return false;
  switch (left.type) {
  case VALUE_NULL:
    return true;
  case VALUE_BOOLEAN:
    return left.as.boolean == right.as.boolean;
  case VALUE_NUMBER:
    return left.as.number == right.as.number;
  case VALUE_DECIMAL:
    return left.as.decimal == right.as.decimal;
  case VALUE_STRING:
    return left.as.string && right.as.string &&
           !strcmp(left.as.string, right.as.string);
  default:
    return false;
  }
}

RuntimeValue valueFunction(RuntimeFunction *function) {
  return (RuntimeValue){.type = VALUE_FUNCTION, .as.function = function};
}

RuntimeValue valueObject(struct RuntimeObjectEntry *entries) {
  return (RuntimeValue){.type = VALUE_OBJECT,
                        .as.object = {.entries = entries}};
}

RuntimeValue valueNativeFunction(const char *name, NativeFn func,
                                 int paramCount) {
  struct RuntimeNativeFunction *nf = calloc(1, sizeof(*nf));
  if (!nf)
    return valueNull();
  nf->name = name;
  nf->func = func;
  nf->paramCount = paramCount;
  return (RuntimeValue){.type = VALUE_NATIVE_FUNCTION, .as.nativeFunc = nf};
}

bool valueObjectGet(RuntimeValue obj, const char *key, RuntimeValue *out) {
  if (obj.type != VALUE_OBJECT || !key)
    return false;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    if (e->key && !strcmp(e->key, key)) {
      if (out)
        *out = e->value;
      return true;
    }
  return false;
}

bool valueObjectSet(RuntimeValue *obj, const char *key, RuntimeValue value) {
  if (!obj || obj->type != VALUE_OBJECT || !key)
    return false;
  /* Update existing entry */
  for (struct RuntimeObjectEntry *e = obj->as.object.entries; e; e = e->next)
    if (e->key && !strcmp(e->key, key)) {
      e->value = value;
      return true;
    }
  /* Add new entry */
  struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
  if (!e)
    return false;
  e->key = strdup(key);
  e->value = value;
  e->next = obj->as.object.entries;
  obj->as.object.entries = e;
  return true;
}

const char *valueTypeName(ValueType type) {
  switch (type) {
  case VALUE_NULL:
    return "null";
  case VALUE_NUMBER:
    return "number";
  case VALUE_DECIMAL:
    return "decimal";
  case VALUE_BOOLEAN:
    return "boolean";
  case VALUE_STRING:
    return "string";
  case VALUE_ARRAY:
    return "array";
  case VALUE_FUNCTION:
    return "function";
  case VALUE_OBJECT:
    return "object";
  case VALUE_NATIVE_FUNCTION:
    return "function";
  default:
    return "unknown";
  }
}
