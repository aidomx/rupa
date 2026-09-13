#include <rupa.h>

InterpreterResult interpretSubscript(Node *node, AstNode *ast, RuntimeEnv *env,
                                     Error *error) {
  if (!node || !ast || ast->type != NODE_SUBSCRIPT) return resultNormal(valueNull());

  InterpreterResult target = interpretNode(node, ast->subscript.posId, env, error);
  if (target.flow != FLOW_NORMAL) return target;

  InterpreterResult index = interpretNode(node, ast->subscript.index, env, error);
  if (index.flow != FLOW_NORMAL) return index;

  /* String indexing: "abc"[1] -> "b" */
  if (target.value.type == VALUE_STRING) {
    const char *text = target.value.as.string;
    if (!text) return resultNormal(valueNull());

    if (index.value.type != VALUE_NUMBER) {
      static char message[256];
      snprintf(message, sizeof(message),
               "string index must be a number, got '%s'",
               valueTypeName(index.value.type));
      if (error)
        addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                     .line = 0, .row = 0, .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    int length = (int)strlen(text);
    int i = index.value.as.number;
    if (i < 0 || i >= length) {
      static char message[256];
      snprintf(message, sizeof(message),
               "index %d is out of bounds for string of length %d", i, length);
      if (error)
        addError(error, (ErrorInfo){.code = "RangeError", .message = message,
                                     .line = 0, .row = 0,
                                     .type = ERR_INDEX_OUT_OF_BOUNDS});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    char ch[2] = {text[i], '\0'};
    return resultNormal(valueString(ch));
  }

  /* Object indexing by string key: obj["name"] */
  if (target.value.type == VALUE_OBJECT) {
    if (index.value.type != VALUE_STRING || !index.value.as.string) {
      static char message[256];
      snprintf(message, sizeof(message),
               "object index must be a string, got '%s'",
               valueTypeName(index.value.type));
      if (error)
        addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                     .line = 0, .row = 0, .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }
    RuntimeValue out;
    if (valueObjectGet(target.value, index.value.as.string, &out))
      return resultNormal(out);
    return resultNormal(valueNull());
  }

  if (target.value.type != VALUE_ARRAY) {
    static char message[256];
    snprintf(message, sizeof(message),
             "cannot index into value of type '%s'",
             valueTypeName(target.value.type));
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                   .line = 0, .row = 0, .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  if (index.value.type != VALUE_NUMBER) {
    static char message[256];
    snprintf(message, sizeof(message),
             "array index must be a number, got '%s'",
             valueTypeName(index.value.type));
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                   .line = 0, .row = 0, .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  int i = index.value.as.number;
  int length = target.value.as.array.length;

  if (i < 0 || i >= length) {
    static char message[256];
    snprintf(message, sizeof(message),
             "index %d is out of bounds for array of length %d", i, length);
    if (error)
      addError(error, (ErrorInfo){.code = "RangeError", .message = message,
                                   .line = 0, .row = 0,
                                   .type = ERR_INDEX_OUT_OF_BOUNDS});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  return resultNormal(target.value.as.array.items[i]);
}
