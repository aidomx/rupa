#include <rupa.h>

InterpreterResult interpretFunction(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  (void)error;
  if (!node || !ast || ast->type != NODE_FUNCTION_DECL)
    return resultNormal(valueNull());

  RuntimeFunction *function = calloc(1, sizeof(*function));
  if (!function) return resultFlow(FLOW_ERROR, valueNull());

  function->node = node;
  function->name = ast->function.name;
  function->params = ast->function.params;
  function->paramLength = ast->function.paramLength;
  function->body = ast->function.body;
  function->closure = env;

  if (function->name >= 0 && function->name < node->length) {
    AstNode *name = &node->ast[function->name];
    if (name->type == NODE_IDENTIFIER && name->identifier.name)
      semSet(env, name->identifier.name, valueFunction(function));
  }

  return resultNormal(valueFunction(function));
}
