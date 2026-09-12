#include <rupa.h>

static bool formatType(Node *node, int id, char *buffer, size_t capacity) {
  if (!node || id < 0 || id >= node->length || !buffer || capacity == 0) return false;

  AstNode *type = &node->ast[id];
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
  if (!formatType(node, type->arrayType.elementType, element, sizeof(element))) return false;
  int written = snprintf(buffer, capacity, "%s[]", element);
  return written > 0 && (size_t)written < capacity;
}

static const char *annotationName(Node *node, int id) {
  static char typeName[256];
  return formatType(node, id, typeName, sizeof(typeName)) ? typeName : NULL;
}

static bool matchesScalar(const char *type, RuntimeValue value) {
  if (!type) return true;
  if (!strcmp(type, "number")) return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL;
  if (!strcmp(type, "decimal")) return value.type == VALUE_DECIMAL;
  if (!strcmp(type, "string")) return value.type == VALUE_STRING;
  if (!strcmp(type, "boolean")) return value.type == VALUE_BOOLEAN;
  if (!strcmp(type, "array")) return value.type == VALUE_ARRAY;
  if (!strcmp(type, "object")) return value.type == VALUE_OBJECT;
  if (!strcmp(type, "null")) return value.type == VALUE_NULL;
  if (!strcmp(type, "function"))
    return value.type == VALUE_FUNCTION || value.type == VALUE_NATIVE_FUNCTION;
  return true;
}

static bool matchesNode(Node *node, int typeId, RuntimeValue value, Error *error) {
  if (!node || typeId < 0 || typeId >= node->length) return true;

  AstNode *type = &node->ast[typeId];
  if (type->type != NODE_ARRAY_TYPE) return matchesScalar(annotationName(node, typeId), value);

  if (value.type != VALUE_ARRAY) {
    if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "array", valueTypeName(value.type));
    return false;
  }

  for (int i = 0; i < value.as.array.length; i++) {
    if (!matchesNode(node, type->arrayType.elementType, value.as.array.items[i], error)) {
      if (error && error->size == 0)
        addRuntimeError(error, ERR_TYPE_MISMATCH, "array element",
                        valueTypeName(value.as.array.items[i].type));
      return false;
    }
  }
  return true;
}

static bool matchesTypeName(const char *type, RuntimeValue value, Error *error) {
  if (!type) return true;

  size_t length = strlen(type);
  if (length >= 2 && type[length - 2] == '[' && type[length - 1] == ']') {
    if (value.type != VALUE_ARRAY) {
      if (error) addRuntimeError(error, ERR_TYPE_MISMATCH, "array", valueTypeName(value.type));
      return false;
    }

    size_t elementLength = length - 2;
    char *elementType = malloc(elementLength + 1);
    if (!elementType) return false;
    memcpy(elementType, type, elementLength);
    elementType[elementLength] = '\0';

    bool valid = true;
    for (int i = 0; i < value.as.array.length; i++) {
      if (!matchesTypeName(elementType, value.as.array.items[i], error)) {
        valid = false;
        break;
      }
    }
    free(elementType);
    return valid;
  }

  return matchesScalar(type, value);
}

bool validateTypeName(const char *type, RuntimeValue value, Error *error) {
  if (matchesTypeName(type, value, error)) return true;
  if (error && (!type || !strstr(type, "[]")))
    addRuntimeError(error, ERR_TYPE_MISMATCH, type, valueTypeName(value.type));
  return false;
}

bool validateAnnotation(Node *node, int typeId, RuntimeValue value, Error *error) {
  return matchesNode(node, typeId, value, error);
}
