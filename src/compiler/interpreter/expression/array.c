#include <rupa.h>

InterpreterResult interpretArray(Node *node, AstNode *ast, RuntimeEnv *env,
                                 Error *error) {
  int length = ast->array.length;
  RuntimeValue *items = length ? calloc((size_t)length, sizeof(*items)) : NULL;
  if (length && !items)
    return resultNormal(valueNull());
  for (int i = 0; i < length; i++) {
    InterpreterResult result =
        interpretNode(node, ast->array.elements[i], env, error);
    items[i] = result.value;
    if (result.flow != FLOW_NORMAL)
      return result;
  }
  return resultNormal(valueArray(items, length));
}
