#include <rupa.h>

/* ==================== Validation helpers ==================== */

static InterpreterResult regexError(Error *error, const char *name, const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"RegexError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  (void)name;
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== regex.match(pattern, str) ==================== */

static InterpreterResult regexMatch(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return regexError(error, "match", "regex.match() expects pattern and string");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return regexError(error, "match", "regex.match() pattern must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return regexError(error, "match", "regex.match() string must be string");

  regex_t regex;
  int ret = regcomp(&regex, argv[0].as.string, REG_EXTENDED | REG_NOSUB);
  if (ret != 0) {
    char buf[256];
    regerror(ret, &regex, buf, sizeof(buf));
    regfree(&regex);
    return regexError(error, "match", buf);
  }
  ret = regexec(&regex, argv[1].as.string, 0, NULL, 0);
  regfree(&regex);
  return resultNormal(valueBoolean(ret == 0));
}

/* ==================== regex.find(pattern, str) ==================== */

static InterpreterResult regexFind(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return regexError(error, "find", "regex.find() expects pattern and string");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return regexError(error, "find", "regex.find() pattern must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return regexError(error, "find", "regex.find() string must be string");

  regex_t regex;
  int ret = regcomp(&regex, argv[0].as.string, REG_EXTENDED);
  if (ret != 0) {
    char buf[256];
    regerror(ret, &regex, buf, sizeof(buf));
    regfree(&regex);
    return regexError(error, "find", buf);
  }

  regmatch_t match[1];
  ret = regexec(&regex, argv[1].as.string, 1, match, 0);
  regfree(&regex);

  if (ret != 0) return resultNormal(valueNull());

  int len = match[0].rm_eo - match[0].rm_so;
  char *result = gcmall(len + 1);
  memcpy(result, argv[1].as.string + match[0].rm_so, len);
  result[len] = '\0';
  return resultNormal(valueString(result));
}

/* ==================== regex.findAll(pattern, str) ==================== */

static InterpreterResult regexFindAll(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return regexError(error, "findAll", "regex.findAll() expects pattern and string");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return regexError(error, "findAll", "regex.findAll() pattern must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return regexError(error, "findAll", "regex.findAll() string must be string");

  regex_t regex;
  int ret = regcomp(&regex, argv[0].as.string, REG_EXTENDED);
  if (ret != 0) {
    char buf[256];
    regerror(ret, &regex, buf, sizeof(buf));
    regfree(&regex);
    return regexError(error, "findAll", buf);
  }

  /* Collect matches into dynamic array */
  int capacity = 16;
  int count = 0;
  RuntimeValue *items = calloc(capacity, sizeof(RuntimeValue));

  const char *str = argv[1].as.string;
  regmatch_t match[1];

  while (regexec(&regex, str, 1, match, 0) == 0) {
    int len = match[0].rm_eo - match[0].rm_so;
    if (len == 0) {
      str++;
      if (*str == '\0') break;
      continue;
    }
    char *matched = gcmall(len + 1);
    memcpy(matched, str + match[0].rm_so, len);
    matched[len] = '\0';

    if (count >= capacity) {
      capacity *= 2;
      items = realloc(items, capacity * sizeof(RuntimeValue));
    }
    items[count++] = valueString(matched);
    str += match[0].rm_eo;
  }

  regfree(&regex);
  return resultNormal(valueArray(items, count));
}

/* ==================== regex.replace(pattern, str, replacement, global?) === */

static InterpreterResult regexReplace(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 3)
    return regexError(error, "replace", "regex.replace() expects pattern, string, replacement");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return regexError(error, "replace", "regex.replace() pattern must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return regexError(error, "replace", "regex.replace() string must be string");
  if (argv[2].type != VALUE_STRING || !argv[2].as.string)
    return regexError(error, "replace", "regex.replace() replacement must be string");

  int global = (argc >= 4 && argv[3].type == VALUE_BOOLEAN && argv[3].as.boolean);

  regex_t regex;
  int ret = regcomp(&regex, argv[0].as.string, REG_EXTENDED);
  if (ret != 0) {
    char buf[256];
    regerror(ret, &regex, buf, sizeof(buf));
    regfree(&regex);
    return regexError(error, "replace", buf);
  }

  const char *str = argv[1].as.string;
  const char *repl = argv[2].as.string;
  int strLen = strlen(str);
  int replLen = strlen(repl);
  int resultCap = strLen * 2 + 64;
  char *result = gcmall(resultCap);
  int pos = 0;
  regmatch_t match[1];

  while (regexec(&regex, str, 1, match, 0) == 0) {
    int before = match[0].rm_so;
    if (pos + before + replLen + 1 >= resultCap) {
      resultCap = (pos + before + replLen + 1) * 2;
      result = realloc(result, resultCap);
    }
    memcpy(result + pos, str, before);
    pos += before;
    memcpy(result + pos, repl, replLen);
    pos += replLen;
    str += match[0].rm_eo;
    if (!global) break;
  }

  int remaining = strlen(str);
  if (pos + remaining + 1 >= resultCap) {
    resultCap = pos + remaining + 1;
    result = realloc(result, resultCap);
  }
  memcpy(result + pos, str, remaining);
  pos += remaining;
  result[pos] = '\0';

  regfree(&regex);
  return resultNormal(valueString(result));
}

/* ==================== regex.split(pattern, str) ==================== */

static InterpreterResult regexSplit(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return regexError(error, "split", "regex.split() expects pattern and string");
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return regexError(error, "split", "regex.split() pattern must be string");
  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return regexError(error, "split", "regex.split() string must be string");

  regex_t regex;
  int ret = regcomp(&regex, argv[0].as.string, REG_EXTENDED);
  if (ret != 0) {
    char buf[256];
    regerror(ret, &regex, buf, sizeof(buf));
    regfree(&regex);
    return regexError(error, "split", buf);
  }

  int capacity = 16;
  int count = 0;
  RuntimeValue *items = calloc(capacity, sizeof(RuntimeValue));

  const char *str = argv[1].as.string;
  regmatch_t match[1];

  while (regexec(&regex, str, 1, match, 0) == 0) {
    int len = match[0].rm_so;
    char *part = gcmall(len + 1);
    memcpy(part, str, len);
    part[len] = '\0';

    if (count >= capacity) {
      capacity *= 2;
      items = realloc(items, capacity * sizeof(RuntimeValue));
    }
    items[count++] = valueString(part);
    str += match[0].rm_eo;
  }

  /* Add remaining */
  int remaining = strlen(str);
  char *part = gcmall(remaining + 1);
  memcpy(part, str, remaining);
  part[remaining] = '\0';
  if (count >= capacity) {
    capacity *= 2;
    items = realloc(items, capacity * sizeof(RuntimeValue));
  }
  items[count++] = valueString(part);

  regfree(&regex);
  return resultNormal(valueArray(items, count));
}

/* ==================== Module init ==================== */

InterpreterResult stdRegexInit(Node *node, int id, RuntimeEnv *env, Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  RuntimeValue mod = valueObject(NULL);
  RuntimeValue fn;

  fn = valueNativeFunction("match", regexMatch, 2);
  valueObjectSet(&mod, "match", fn);

  fn = valueNativeFunction("find", regexFind, 2);
  valueObjectSet(&mod, "find", fn);

  fn = valueNativeFunction("findAll", regexFindAll, 2);
  valueObjectSet(&mod, "findAll", fn);

  fn = valueNativeFunction("replace", regexReplace, 3);
  valueObjectSet(&mod, "replace", fn);

  fn = valueNativeFunction("split", regexSplit, 2);
  valueObjectSet(&mod, "split", fn);

  return resultNormal(mod);
}
