#include <rupa.h>

static InterpreterResult stringTypeError(Error *error, const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"TypeError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  return resultFlow(FLOW_ERROR, valueNull());
}

static bool getRuntimeString(int argc, RuntimeValue *argv, const char **out) {
  if (argc < 1 || !argv || !out || argv[0].type != VALUE_STRING ||
      !argv[0].as.string)
    return false;
  *out = argv[0].as.string;
  return true;
}

InterpreterResult stdStringLength(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value))
    return stringTypeError(error, "string.length() expects a string");
  return resultNormal(valueNumber((int)strlen(value)));
}

InterpreterResult stdStringUpper(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value))
    return stringTypeError(error, "string.upper() expects a string");
  size_t length = strlen(value);
  char *result = malloc(length + 1);
  if (!result)
    return resultNormal(valueNull());
  for (size_t i = 0; i < length; i++)
    result[i] = (char)toupper((unsigned char)value[i]);
  result[length] = '\0';
  RuntimeValue out = valueString(result);
  free(result);
  return resultNormal(out);
}

InterpreterResult stdStringLower(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value))
    return stringTypeError(error, "string.lower() expects a string");
  size_t length = strlen(value);
  char *result = malloc(length + 1);
  if (!result)
    return resultNormal(valueNull());
  for (size_t i = 0; i < length; i++)
    result[i] = (char)tolower((unsigned char)value[i]);
  result[length] = '\0';
  RuntimeValue out = valueString(result);
  free(result);
  return resultNormal(out);
}

InterpreterResult stdStringTrim(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value))
    return stringTypeError(error, "string.trim() expects a string");
  const char *start = value;
  while (*start && isspace((unsigned char)*start))
    start++;
  const char *end = value + strlen(value);
  while (end > start && isspace((unsigned char)end[-1]))
    end--;
  size_t length = (size_t)(end - start);
  char *result = malloc(length + 1);
  if (!result)
    return resultNormal(valueNull());
  memcpy(result, start, length);
  result[length] = '\0';
  RuntimeValue out = valueString(result);
  free(result);
  return resultNormal(out);
}

InterpreterResult stdStringContains(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return stringTypeError(error, "string.contains() expects two strings");
  return resultNormal(valueBoolean(strstr(value, argv[1].as.string) != NULL));
}

InterpreterResult stdStringStartsWith(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return stringTypeError(error, "string.startsWith() expects two strings");
  size_t prefixLength = strlen(argv[1].as.string);
  return resultNormal(
      valueBoolean(strlen(value) >= prefixLength &&
                   !strncmp(value, argv[1].as.string, prefixLength)));
}

InterpreterResult stdStringEndsWith(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return stringTypeError(error, "string.endsWith() expects two strings");
  size_t valueLength = strlen(value), suffixLength = strlen(argv[1].as.string);
  return resultNormal(valueBoolean(
      valueLength >= suffixLength &&
      !strcmp(value + valueLength - suffixLength, argv[1].as.string)));
}

InterpreterResult stdStringReplace(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 3 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string ||
      argv[2].type != VALUE_STRING || !argv[2].as.string)
    return stringTypeError(error, "string.replace() expects three strings");

  const char *from = argv[1].as.string;
  const char *to = argv[2].as.string;
  size_t fromLength = strlen(from), toLength = strlen(to);
  if (fromLength == 0)
    return resultNormal(valueString(value));

  const char *match = strstr(value, from);
  if (!match)
    return resultNormal(valueString(value));
  size_t prefix = (size_t)(match - value);
  size_t suffix = strlen(match + fromLength);
  char *result = malloc(prefix + toLength + suffix + 1);
  if (!result)
    return resultNormal(valueNull());
  memcpy(result, value, prefix);
  memcpy(result + prefix, to, toLength);
  memcpy(result + prefix + toLength, match + fromLength, suffix);
  result[prefix + toLength + suffix] = '\0';
  RuntimeValue out = valueString(result);
  free(result);
  return resultNormal(out);
}

/* ==================== string.split(sep) ==================== */
InterpreterResult stdStringSplit(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string ||
      argv[1].as.string[0] == '\0')
    return stringTypeError(error,
                           "string.split() expects a non-empty separator");

  const char *sep = argv[1].as.string;
  size_t sepLength = strlen(sep);

  /* Count parts first so the items buffer can be allocated once. */
  int count = 1;
  for (const char *p = value; (p = strstr(p, sep)) != NULL; p += sepLength)
    count++;

  RuntimeValue *items = calloc((size_t)count, sizeof(*items));
  if (!items)
    return resultNormal(valueNull());

  int n = 0;
  const char *start = value;
  for (const char *p; (p = strstr(start, sep)) != NULL;) {
    size_t partLength = (size_t)(p - start);
    char *part = malloc(partLength + 1);
    if (!part) {
      free(items);
      return resultNormal(valueNull());
    }
    memcpy(part, start, partLength);
    part[partLength] = '\0';
    items[n++] = valueString(part);
    free(part);
    start = p + sepLength;
  }
  items[n++] = valueString(start); /* remainder after the last separator */

  return resultNormal(valueArray(items, n));
}

/* ==================== string.indexOf(sub) ==================== */
InterpreterResult stdStringIndexOf(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return stringTypeError(error, "string.indexOf() expects a string");

  if (argv[1].as.string[0] == '\0')
    return resultNormal(valueNumber(0));

  const char *found = strstr(value, argv[1].as.string);
  return resultNormal(valueNumber(found ? (int)(found - value) : -1));
}

/* ==================== string.slice(start, end?) ==================== */
InterpreterResult stdStringSlice(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  const char *value;
  (void)env;
  if (!getRuntimeString(argc, argv, &value) || argc < 2 ||
      argv[1].type != VALUE_NUMBER)
    return stringTypeError(error,
                           "string.slice() expects (start, end?) numbers");

  int length = (int)strlen(value);
  int start = argv[1].as.number;
  int end = (argc >= 3 && argv[2].type == VALUE_NUMBER) ? argv[2].as.number
                                                        : length;

  /* Negative indices count from the end, then clamp into [0, length]. */
  if (start < 0)
    start = length + start;
  if (end < 0)
    end = length + end;
  if (start < 0)
    start = 0;
  if (end > length)
    end = length;
  if (start > length)
    start = length;
  if (start >= end)
    return resultNormal(valueString(""));

  char *result = malloc((size_t)(end - start) + 1);
  if (!result)
    return resultNormal(valueNull());
  memcpy(result, value + start, (size_t)(end - start));
  result[end - start] = '\0';
  RuntimeValue out = valueString(result);
  free(result);
  return resultNormal(out);
}

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

InterpreterResult stdStringInit(Node *node, int id, RuntimeEnv *env,
                                Error *error) {
  struct RuntimeObjectEntry *entries = NULL;
  (void)node;
  (void)id;
  (void)env;
  (void)error;
  addEntry(&entries, "length", stdStringLength, 1);
  addEntry(&entries, "upper", stdStringUpper, 1);
  addEntry(&entries, "lower", stdStringLower, 1);
  addEntry(&entries, "trim", stdStringTrim, 1);
  addEntry(&entries, "contains", stdStringContains, 2);
  addEntry(&entries, "startsWith", stdStringStartsWith, 2);
  addEntry(&entries, "endsWith", stdStringEndsWith, 2);
  addEntry(&entries, "replace", stdStringReplace, 3);
  addEntry(&entries, "split", stdStringSplit, 2);
  addEntry(&entries, "indexOf", stdStringIndexOf, 2);
  addEntry(&entries, "slice", stdStringSlice, 3);
  return resultNormal(valueObject(entries));
}
