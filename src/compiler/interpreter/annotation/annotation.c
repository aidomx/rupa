#include <rupa.h>

static const char *annotationName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length)
    return NULL;

  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER)
    return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID)
    return ast->string.value;
  return NULL;
}

static bool matches(const char *type, RuntimeValue value) {
  if (!type)
    return true;

  if (!strcmp(type, "number"))
    return value.type == VALUE_NUMBER || value.type == VALUE_DECIMAL;
  if (!strcmp(type, "decimal"))
    return value.type == VALUE_DECIMAL;
  if (!strcmp(type, "string"))
    return value.type == VALUE_STRING;
  if (!strcmp(type, "boolean"))
    return value.type == VALUE_BOOLEAN;
  if (!strcmp(type, "array"))
    return value.type == VALUE_ARRAY;
  if (!strcmp(type, "null"))
    return value.type == VALUE_NULL;
  if (!strcmp(type, "function"))
    return value.type == VALUE_FUNCTION;

  return true; /* unknown/custom type is not enforced yet */
}

bool validateTypeName(const char *type, RuntimeValue value, Error *error) {
  if (matches(type, value)) return true;
  if (error)
    addRuntimeError(error, ERR_TYPE_MISMATCH, type, valueTypeName(value.type));
  return false;
}

bool validateAnnotation(Node *node, int typeId, RuntimeValue value,
                        Error *error) {
  return validateTypeName(annotationName(node, typeId), value, error);
}
