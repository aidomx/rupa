#include <rupa.h>

static const char *targetName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  return NULL;
}

InterpreterResult interpretUpdate(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  if (!node || !ast || ast->type != NODE_UPDATE) return resultNormal(valueNull());

  const char *name = targetName(node, ast->update.target);
  if (!name) {
    if (error)
      addError(error, (ErrorInfo){.code = "SyntaxError",
                                   .message = "invalid update target",
                                   .line = 0, .row = 0, .type = ERR_SYNTAX});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  RuntimeValue current;
  if (!semGet(env, name, &current)) {
    static char message[256];
    snprintf(message, sizeof(message), "'%s' is not defined", name);
    if (error)
      addError(error, (ErrorInfo){.code = "ReferenceError", .message = message,
                                   .line = 0, .row = 0, .type = ERR_UNDEFINED_VAR});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  bool isIncrement = ast->update.op && !strcmp(ast->update.op, "++");
  RuntimeValue updated;

  if (current.type == VALUE_DECIMAL)
    updated = valueDecimal(current.as.decimal + (isIncrement ? 1 : -1));
  else if (current.type == VALUE_NUMBER)
    updated = valueNumber(current.as.number + (isIncrement ? 1 : -1));
  else {
    static char message[256];
    snprintf(message, sizeof(message),
             "cannot apply '%s' to value of type '%s'",
             ast->update.op ? ast->update.op : "?",
             valueTypeName(current.type));
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                   .line = 0, .row = 0, .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  semSet(env, name, updated);

  /* Prefix returns the new value, postfix returns the old value. */
  return resultNormal(ast->update.prefix ? updated : current);
}
