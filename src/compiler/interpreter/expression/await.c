#include <rupa.h>

/* Await evaluates its operand and, if the result is an async handle object,
 * extracts the .data field. If the operand is not an object, it is returned
 * as-is (await on a plain value is a no-op). */
InterpreterResult interpretAwait(Node *node, AstNode *ast, RuntimeEnv *env,
                                 Error *error) {
  if (!node || !ast || ast->type != NODE_AWAIT)
    return resultNormal(valueNull());

  InterpreterResult inner = interpretNode(node, ast->await.expression, env, error);
  if (inner.flow != FLOW_NORMAL) return inner;

  if (inner.value.type == VALUE_OBJECT) {
    RuntimeValue data;
    if (valueObjectGet(inner.value, "data", &data))
      return resultNormal(data);
  }

  /* Not an async handle — return as-is */
  return resultNormal(inner.value);
}
