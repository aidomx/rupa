#include <rupa.h>

static const char *paramName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  if (ast->type == NODE_ANNOTATION) {
    int name = ast->annotation.name;
    if (name >= 0 && name < node->length && node->ast[name].type == NODE_IDENTIFIER)
      return node->ast[name].identifier.name;
  }
  return NULL;
}

static const char *calleeName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  return NULL;
}

InterpreterResult interpretCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  InterpreterResult callee = interpretNode(node, ast->call.callee, env, error);
  if (callee.flow != FLOW_NORMAL) return callee;

  /* Handle native function calls */
  if (callee.value.type == VALUE_NATIVE_FUNCTION && callee.value.as.nativeFunc) {
    struct RuntimeNativeFunction *nf = callee.value.as.nativeFunc;
    int explicitArgc = ast->call.length;
    int argc = explicitArgc + (nf->hasReceiver ? 1 : 0);
    RuntimeValue *argv = calloc(argc, sizeof(RuntimeValue));
    if (!argv && argc > 0) {
      if (error)
        addError(error, (ErrorInfo){.code = "InternalError",
                                   .message = "failed to allocate argument list",
                                   .line = 0, .row = 0, .type = ERR_INTERNAL});
      return resultFlow(FLOW_ERROR, valueNull());
    }
    int offset = 0;
    if (nf->hasReceiver) {
      argv[0] = nf->receiver ? *nf->receiver : valueNull();
      offset = 1;
    }
    for (int i = 0; i < explicitArgc; i++) {
      InterpreterResult arg = interpretNode(node, ast->call.args[i], env, error);
      if (arg.flow != FLOW_NORMAL) { free(argv); return arg; }
      argv[i + offset] = arg.value;
    }
    InterpreterResult result = nf->func(argc, argv, env, error);
    free(argv);
    return result;
  }

  if (callee.value.type != VALUE_FUNCTION || !callee.value.as.function) {
    if (error) {
      const char *name = calleeName(node, ast->call.callee);
      static char message[256];
      if (name)
        snprintf(message, sizeof(message),
                 "'%s' is not a function and cannot be called", name);
      else
        snprintf(message, sizeof(message),
                 "value of type '%s' is not a function and cannot be called",
                 valueTypeName(callee.value.type));
      addError(error, (ErrorInfo){.code = "TypeError", .message = message,
                                   .line = 0, .row = 0,
                                   .type = ERR_INVALID_CALL});
    }
    return resultFlow(FLOW_ERROR, valueNull());
  }

  RuntimeFunction *function = callee.value.as.function;
  RuntimeEnv *local = semCreateEnv(function->closure);
  if (!local) {
    if (error)
      addError(error, (ErrorInfo){.code = "InternalError",
                                   .message = "failed to allocate call frame",
                                   .line = 0, .row = 0, .type = ERR_INTERNAL});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  int count = ast->call.length < function->paramLength ? ast->call.length : function->paramLength;
  for (int i = 0; i < count; i++) {
    InterpreterResult arg = interpretNode(node, ast->call.args[i], env, error);
    if (arg.flow != FLOW_NORMAL) return arg;
    const char *name = paramName(function->node, function->params[i]);
    if (name) semSet(local, name, arg.value);
  }

  InterpreterResult result = interpretNode(function->node, function->body, local, error);
  if (result.flow == FLOW_RETURN) return resultNormal(result.value);
  return result;
}
