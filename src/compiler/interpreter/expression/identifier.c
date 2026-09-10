#include <rupa.h>

InterpreterResult interpretIdentifier(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  if (!node || !ast || !env) return (InterpreterResult){0};

  RuntimeValue value;

  const char *name = ast->type == NODE_IDENTIFIER ? ast->identifier.name : ast->string.value;

  if (name && semGet(env, name, &value)) return resultNormal(value);

  if (name) {
    static char message[256];
    snprintf(message, sizeof(message), "%s is not defined", name);
    if (error)
      addError(error, (ErrorInfo){.code = "ReferenceError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_UNDEFINED_VAR});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  return resultNormal(valueNull());
}
