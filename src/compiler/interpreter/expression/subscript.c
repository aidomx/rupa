#include <rupa.h>

InterpreterResult interpretSubscript(Node *node, AstNode *ast, RuntimeEnv *env,
                                     Error *error) {
  if (!node || !ast || ast->type != NODE_SUBSCRIPT) return resultNormal(valueNull());

  InterpreterResult target = interpretNode(node, ast->subscript.posId, env, error);
  if (target.flow != FLOW_NORMAL) return target;

  InterpreterResult index = interpretNode(node, ast->subscript.index, env, error);
  if (index.flow != FLOW_NORMAL) return index;

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
