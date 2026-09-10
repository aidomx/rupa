#include <rupa.h>

/* JSON Parser — extracted from json.c for modularity.
 * This file contains all JSON parsing logic (tokenizer + recursive descent).
 * Stringify and module registration remain in json.c. */

void skipJsonWhitespace(JsonParser *parser) {
  while (isspace((unsigned char)parser->text[parser->position]))
    parser->position++;
}

bool consume(JsonParser *parser, char expected) {
  skipJsonWhitespace(parser);
  if (parser->text[parser->position] != expected)
    return false;
  parser->position++;
  return true;
}

RuntimeValue parseJsonString(JsonParser *parser, bool *ok) {
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

RuntimeValue parseJsonNumber(JsonParser *parser, bool *ok) {
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

RuntimeValue parseJsonArray(JsonParser *parser, bool *ok) {
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

RuntimeValue parseJsonObject(JsonParser *parser, bool *ok) {
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

RuntimeValue parseJsonValue(JsonParser *parser, bool *ok) {
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
