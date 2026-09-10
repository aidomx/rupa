#include <rupa.h>

/* JSON Stringify + Module Init — parser extracted to json_parser.c */

static InterpreterResult jsonTypeError(Error *error, const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"TypeError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  return resultFlow(FLOW_ERROR, valueNull());
}

static void appendText(char **buffer, size_t *length, size_t *capacity,
                       const char *text) {
  size_t add = strlen(text);
  if (*length + add + 1 > *capacity) {
    while (*length + add + 1 > *capacity)
      *capacity *= 2;
    *buffer = realloc(*buffer, *capacity);
  }
  memcpy(*buffer + *length, text, add);
  *length += add;
  (*buffer)[*length] = '\0';
}

static void appendChar(char **buffer, size_t *length, size_t *capacity,
                       char value) {
  if (*length + 2 > *capacity) {
    *capacity *= 2;
    *buffer = realloc(*buffer, *capacity);
  }
  (*buffer)[(*length)++] = value;
  (*buffer)[*length] = '\0';
}

static void stringifyValue(RuntimeValue value, char **buffer, size_t *length,
                           size_t *capacity) {
  char number[64];
  switch (value.type) {
  case VALUE_NULL:
    appendText(buffer, length, capacity, "null");
    break;
  case VALUE_BOOLEAN:
    appendText(buffer, length, capacity, value.as.boolean ? "true" : "false");
    break;
  case VALUE_NUMBER:
    snprintf(number, sizeof(number), "%d", value.as.number);
    appendText(buffer, length, capacity, number);
    break;
  case VALUE_DECIMAL:
    snprintf(number, sizeof(number), "%g", value.as.decimal);
    appendText(buffer, length, capacity, number);
    break;
  case VALUE_STRING:
    appendChar(buffer, length, capacity, '"');
    if (value.as.string) {
      for (const char *p = value.as.string; *p; p++) {
        if (*p == '"' || *p == '\\')
          appendChar(buffer, length, capacity, '\\');
        appendChar(buffer, length, capacity, *p);
      }
    }
    appendChar(buffer, length, capacity, '"');
    break;
  case VALUE_ARRAY:
    appendChar(buffer, length, capacity, '[');
    for (int i = 0; i < value.as.array.length; i++) {
      if (i > 0)
        appendChar(buffer, length, capacity, ',');
      stringifyValue(value.as.array.items[i], buffer, length, capacity);
    }
    appendChar(buffer, length, capacity, ']');
    break;
  case VALUE_OBJECT:
    appendChar(buffer, length, capacity, '{');
    {
      bool first = true;
      for (struct RuntimeObjectEntry *entry = value.as.object.entries; entry;
           entry = entry->next) {
        if (!first)
          appendChar(buffer, length, capacity, ',');
        first = false;
        appendChar(buffer, length, capacity, '"');
        if (entry->key) {
          for (const char *p = entry->key; *p; p++) {
            if (*p == '"' || *p == '\\')
              appendChar(buffer, length, capacity, '\\');
            appendChar(buffer, length, capacity, *p);
          }
        }
        appendChar(buffer, length, capacity, '"');
        appendChar(buffer, length, capacity, ':');
        stringifyValue(entry->value, buffer, length, capacity);
      }
    }
    appendChar(buffer, length, capacity, '}');
    break;
  default:
    appendText(buffer, length, capacity, "null");
    break;
  }
}

InterpreterResult jsonStringify(int argc, RuntimeValue *argv,
                               RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv)
    return jsonTypeError(error, "json.stringify() expects a value");
  size_t length = 0, capacity = 128;
  char *buffer = calloc(capacity, 1);
  if (!buffer)
    return resultNormal(valueNull());
  stringifyValue(argv[0], &buffer, &length, &capacity);
  RuntimeValue result = valueString(buffer);
  free(buffer);
  return resultNormal(result);
}

static InterpreterResult jsonParse(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return jsonTypeError(error, "json.parse() expects a JSON string");
  JsonParser parser = {.text = argv[0].as.string, .position = 0};
  bool ok = true;
  RuntimeValue result = parseJsonValue(&parser, &ok);
  skipJsonWhitespace(&parser);
  if (!ok || parser.text[parser.position] != '\0')
    return jsonTypeError(error, "json.parse() received invalid JSON");
  return resultNormal(result);
}

/* ---- Utility functions ---- */

static InterpreterResult jsonValid(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return jsonTypeError(error, "json.valid() expects a string");

  JsonParser parser = {.text = argv[0].as.string, .position = 0};
  bool ok = true;
  RuntimeValue result = parseJsonValue(&parser, &ok);
  (void)result;
  skipJsonWhitespace(&parser);
  if (!ok || parser.text[parser.position] != '\0')
    return resultNormal(valueBoolean(false));
  return resultNormal(valueBoolean(true));
}

static InterpreterResult jsonKeys(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv)
    return jsonTypeError(error, "json.keys() expects an object");

  RuntimeValue obj = argv[0];
  if (obj.type != VALUE_OBJECT)
    return jsonTypeError(error, "json.keys() expects an object");

  int count = 0;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    count++;

  RuntimeValue *items = calloc(count, sizeof(RuntimeValue));
  int i = 0;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    items[i++] = valueString(e->key ? e->key : "");

  return resultNormal(valueArray(items, count));
}

static InterpreterResult jsonValues(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv)
    return jsonTypeError(error, "json.values() expects an object");

  RuntimeValue obj = argv[0];
  if (obj.type != VALUE_OBJECT)
    return jsonTypeError(error, "json.values() expects an object");

  int count = 0;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    count++;

  RuntimeValue *items = calloc(count, sizeof(RuntimeValue));
  int i = 0;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    items[i++] = e->value;

  return resultNormal(valueArray(items, count));
}

static InterpreterResult jsonMerge(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || !argv)
    return jsonTypeError(error, "json.merge() expects two objects");

  if (argv[0].type != VALUE_OBJECT || argv[1].type != VALUE_OBJECT)
    return jsonTypeError(error, "json.merge() expects two objects");

  struct RuntimeObjectEntry *entries = NULL, **tail = &entries;
  for (struct RuntimeObjectEntry *e = argv[0].as.object.entries; e;
       e = e->next) {
    struct RuntimeObjectEntry *ne = calloc(1, sizeof(*ne));
    ne->key = strdup(e->key ? e->key : "");
    ne->value = e->value;
    *tail = ne;
    tail = &ne->next;
  }

  for (struct RuntimeObjectEntry *e = argv[1].as.object.entries; e;
       e = e->next) {
    bool found = false;
    for (struct RuntimeObjectEntry *ne = entries; ne; ne = ne->next) {
      if (ne->key && e->key && strcmp(ne->key, e->key) == 0) {
        ne->value = e->value;
        found = true;
        break;
      }
    }
    if (!found) {
      struct RuntimeObjectEntry *ne = calloc(1, sizeof(*ne));
      ne->key = strdup(e->key ? e->key : "");
      ne->value = e->value;
      *tail = ne;
      tail = &ne->next;
    }
  }

  return resultNormal(valueObject(entries));
}

static InterpreterResult jsonGet(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || !argv)
    return jsonTypeError(error, "json.get() expects (object, path)");

  if (argv[0].type != VALUE_OBJECT)
    return jsonTypeError(error, "json.get() expects an object as first argument");

  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return jsonTypeError(error, "json.get() expects a string path");

  const char *path = argv[1].as.string;
  RuntimeValue current = argv[0];

  const char *p = path;
  while (*p && current.type == VALUE_OBJECT) {
    const char *start = p;
    while (*p && *p != '.')
      p++;
    size_t segLen = (size_t)(p - start);

    bool found = false;
    for (struct RuntimeObjectEntry *e = current.as.object.entries; e;
         e = e->next) {
      if (e->key && strlen(e->key) == segLen &&
          strncmp(e->key, start, segLen) == 0) {
        current = e->value;
        found = true;
        break;
      }
    }
    if (!found)
      return resultNormal(valueNull());

    if (*p == '.')
      p++;
  }

  return resultNormal(current);
}

/* ---- Module registration ---- */

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

InterpreterResult stdJsonInit(Node *node, int id, RuntimeEnv *env,
                              Error *error) {
  struct RuntimeObjectEntry *entries = NULL;
  (void)node;
  (void)id;
  (void)env;
  (void)error;
  addEntry(&entries, "stringify", jsonStringify, 1);
  addEntry(&entries, "parse", jsonParse, 1);
  addEntry(&entries, "valid", jsonValid, 1);
  addEntry(&entries, "keys", jsonKeys, 1);
  addEntry(&entries, "values", jsonValues, 1);
  addEntry(&entries, "merge", jsonMerge, 2);
  addEntry(&entries, "get", jsonGet, 2);
  return resultNormal(valueObject(entries));
}
