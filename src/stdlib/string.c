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
  return resultNormal(valueObject(entries));
}
