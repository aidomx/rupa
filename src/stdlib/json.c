#include <rupa.h>

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
    snprintf(number, sizeof(number), "%.17g", value.as.decimal);
    appendText(buffer, length, capacity, number);
    break;
  case VALUE_STRING:
    appendChar(buffer, length, capacity, '"');
    for (const char *p = value.as.string ? value.as.string : ""; *p; p++) {
      switch (*p) {
      case '"':
        appendText(buffer, length, capacity, "\\\"");
        break;
      case '\\':
        appendText(buffer, length, capacity, "\\\\");
        break;
      case '\n':
        appendText(buffer, length, capacity, "\\n");
        break;
      case '\r':
        appendText(buffer, length, capacity, "\\r");
        break;
      case '\t':
        appendText(buffer, length, capacity, "\\t");
        break;
      default:
        appendChar(buffer, length, capacity, *p);
        break;
      }
    }
    appendChar(buffer, length, capacity, '"');
    break;
  case VALUE_ARRAY:
    appendChar(buffer, length, capacity, '[');
    for (int i = 0; i < value.as.array.length; i++) {
      if (i)
        appendChar(buffer, length, capacity, ',');
      stringifyValue(value.as.array.items[i], buffer, length, capacity);
    }
    appendChar(buffer, length, capacity, ']');
    break;
  case VALUE_OBJECT: {
    appendChar(buffer, length, capacity, '{');
    bool first = true;
    for (struct RuntimeObjectEntry *entry = value.as.object.entries; entry;
         entry = entry->next) {
      if (!first)
        appendChar(buffer, length, capacity, ',');
      first = false;
      RuntimeValue key = valueString(entry->key ? entry->key : "");
      stringifyValue(key, buffer, length, capacity);
      appendChar(buffer, length, capacity, ':');
      stringifyValue(entry->value, buffer, length, capacity);
    }
    appendChar(buffer, length, capacity, '}');
    break;
  }
  default:
    appendText(buffer, length, capacity, "null");
    break;
  }
}

static InterpreterResult jsonStringify(int argc, RuntimeValue *argv,
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

typedef struct {
  const char *text;
  size_t position;
} JsonParser;

static void skipJsonWhitespace(JsonParser *parser) {
  while (isspace((unsigned char)parser->text[parser->position]))
    parser->position++;
}

static bool consume(JsonParser *parser, char expected) {
  skipJsonWhitespace(parser);
  if (parser->text[parser->position] != expected)
    return false;
  parser->position++;
  return true;
}

static RuntimeValue parseJsonValue(JsonParser *parser, bool *ok);

static RuntimeValue parseJsonString(JsonParser *parser, bool *ok) {
  if (!consume(parser, '"')) {
    *ok = false;
    return valueNull();
  }
  size_t capacity = 32, length = 0;
  char *buffer = calloc(capacity, 1);
  if (!buffer) {
    *ok = false;
    return valueNull();
  }
  while (parser->text[parser->position] &&
         parser->text[parser->position] != '"') {
    char value = parser->text[parser->position++];
    if (value == '\\') {
      value = parser->text[parser->position++];
      if (value == 'n')
        value = '\n';
      else if (value == 'r')
        value = '\r';
      else if (value == 't')
        value = '\t';
      else if (value != '"' && value != '\\' && value != '/') {
        free(buffer);
        *ok = false;
        return valueNull();
      }
    }
    if (length + 2 > capacity) {
      capacity *= 2;
      buffer = realloc(buffer, capacity);
    }
    buffer[length++] = value;
  }
  if (!consume(parser, '"')) {
    free(buffer);
    *ok = false;
    return valueNull();
  }
  buffer[length] = '\0';
  RuntimeValue result = valueString(buffer);
  free(buffer);
  return result;
}

static RuntimeValue parseJsonNumber(JsonParser *parser, bool *ok) {
  char *end;
  errno = 0;
  double value = strtod(parser->text + parser->position, &end);
  if (end == parser->text + parser->position || errno == ERANGE) {
    *ok = false;
    return valueNull();
  }
  parser->position = (size_t)(end - parser->text);
  if (floor(value) == value && value >= INT_MIN && value <= INT_MAX)
    return valueNumber((int)value);
  return valueDecimal(value);
}

static RuntimeValue parseJsonArray(JsonParser *parser, bool *ok) {
  if (!consume(parser, '[')) {
    *ok = false;
    return valueNull();
  }
  int capacity = 8, length = 0;
  RuntimeValue *items = calloc(capacity, sizeof(*items));
  skipJsonWhitespace(parser);
  if (parser->text[parser->position] == ']') {
    parser->position++;
    return valueArray(items, 0);
  }
  while (*ok) {
    if (length >= capacity) {
      capacity *= 2;
      items = realloc(items, capacity * sizeof(*items));
    }
    items[length++] = parseJsonValue(parser, ok);
    skipJsonWhitespace(parser);
    if (parser->text[parser->position] == ']') {
      parser->position++;
      break;
    }
    if (!consume(parser, ',')) {
      *ok = false;
      break;
    }
  }
  if (!*ok) {
    free(items);
    return valueNull();
  }
  return valueArray(items, length);
}

static RuntimeValue parseJsonObject(JsonParser *parser, bool *ok) {
  if (!consume(parser, '{')) {
    *ok = false;
    return valueNull();
  }
  struct RuntimeObjectEntry *entries = NULL, **tail = &entries;
  skipJsonWhitespace(parser);
  if (parser->text[parser->position] == '}') {
    parser->position++;
    return valueObject(entries);
  }
  while (*ok) {
    RuntimeValue key = parseJsonString(parser, ok);
    if (!*ok || !consume(parser, ':')) {
      *ok = false;
      break;
    }
    RuntimeValue value = parseJsonValue(parser, ok);
    if (!*ok || key.type != VALUE_STRING) {
      *ok = false;
      break;
    }
    struct RuntimeObjectEntry *entry = calloc(1, sizeof(*entry));
    if (!entry) {
      *ok = false;
      break;
    }
    entry->key = strdup(key.as.string ? key.as.string : "");
    entry->value = value;
    *tail = entry;
    tail = &entry->next;
    skipJsonWhitespace(parser);
    if (parser->text[parser->position] == '}') {
      parser->position++;
      break;
    }
    if (!consume(parser, ',')) {
      *ok = false;
      break;
    }
  }
  if (!*ok)
    return valueNull();
  return valueObject(entries);
}

static RuntimeValue parseJsonValue(JsonParser *parser, bool *ok) {
  skipJsonWhitespace(parser);
  char first = parser->text[parser->position];
  if (first == '"')
    return parseJsonString(parser, ok);
  if (first == '[')
    return parseJsonArray(parser, ok);
  if (first == '{')
    return parseJsonObject(parser, ok);
  if (!strncmp(parser->text + parser->position, "true", 4)) {
    parser->position += 4;
    return valueBoolean(true);
  }
  if (!strncmp(parser->text + parser->position, "false", 5)) {
    parser->position += 5;
    return valueBoolean(false);
  }
  if (!strncmp(parser->text + parser->position, "null", 4)) {
    parser->position += 4;
    return valueNull();
  }
  if (first == '-' || isdigit((unsigned char)first))
    return parseJsonNumber(parser, ok);
  *ok = false;
  return valueNull();
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
  return resultNormal(valueObject(entries));
}
