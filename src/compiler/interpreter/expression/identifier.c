#include <rupa.h>

InterpreterResult interpretIdentifier(Node *node, AstNode *ast,
                                      RuntimeEnv *env) {
  if (!node || !ast || !env)
    return (InterpreterResult){0};

  RuntimeValue value;

  const char *name =
      ast->type == NODE_IDENTIFIER ? ast->identifier.name : ast->string.value;

  if (name && semGet(env, name, &value))
    return resultNormal(value);

  return resultNormal(valueNull());
}
