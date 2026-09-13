#include <rupa.h>

/* Check if a name is private in any namespace object in the env chain */
static bool isPrivateName(RuntimeEnv *env, const char *name) {
  if (!env || !name) return false;
  for (RuntimeEnv *e = env; e; e = e->parent) {
    for (RuntimeBinding *b = e->bindings; b; b = b->next) {
      if (b->value.type != VALUE_OBJECT) continue;
      /* Check if this object has a _private entry */
      RuntimeValue priv_val;
      if (valueObjectGet(b->value, "_private", &priv_val) &&
          priv_val.type == VALUE_OBJECT) {
        RuntimeValue found;
        if (valueObjectGet(priv_val, name, &found))
          return true;
      }
    }
  }
  return false;
}

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
  /* math.add → return "add" (the member name) */
  if (ast->type == NODE_MEMBER) {
    int mid = ast->member.member;
    if (mid >= 0 && mid < node->length) {
      AstNode *m = &node->ast[mid];
      if (m->type == NODE_IDENTIFIER) return m->identifier.name;
      if (m->type == NODE_LITERAL_ID) return m->string.value;
    }
  }
  return NULL;
}

/*
 * Array builtin methods: push / pop.
 *
 * These mutate the array in place, so they are handled here (with access to
 * the env) instead of as plain native functions: after gcrealloc moves the
 * items buffer, the mutated array must be written back to the variable that
 * holds it, otherwise every other reference keeps a stale pointer/length.
 * Method-style mutators stop at Tier 1 on purpose — map/filter/reduce stay
 * plain functions in stdlib/collections so the core stays minimal.
 */
static InterpreterResult arrayBuiltinCall(Node *node, AstNode *ast,
                                          RuntimeEnv *env, Error *error,
                                          bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  if (calleeAst->type != NODE_MEMBER) return resultNormal(valueNull());

  int mid = calleeAst->member.member;
  if (mid < 0 || mid >= node->length) return resultNormal(valueNull());
  AstNode *m = &node->ast[mid];
  const char *mname = NULL;
  if (m->type == NODE_IDENTIFIER) mname = m->identifier.name;
  else if (m->type == NODE_LITERAL_ID) mname = m->string.value;
  if (!mname) return resultNormal(valueNull());

  bool isPush = !strcmp(mname, "push");
  bool isPop = !strcmp(mname, "pop");
  if (!isPush && !isPop) return resultNormal(valueNull());

  *handled = true;

  InterpreterResult base =
      interpretNode(node, calleeAst->member.object, env, error);
  if (base.flow != FLOW_NORMAL) return base;

  if (base.value.type != VALUE_ARRAY) {
    if (error)
      addError(error,
               (ErrorInfo){.code = "TypeError",
                           .message = mname == NULL ? "array method expects an array"
                                      : isPush ? "push() expects an array"
                                               : "pop() expects an array",
                           .line = 0,
                           .row = 0,
                           .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  RuntimeValue arr = base.value;
  InterpreterResult ret = resultNormal(valueNull());

  if (isPush) {
    /* Append each argument (push() with no args is a no-op). */
    for (int i = 0; i < ast->call.length; i++) {
      InterpreterResult arg = interpretNode(node, ast->call.args[i], env, error);
      if (arg.flow != FLOW_NORMAL) return arg;
      int newLen = arr.as.array.length + 1;
      RuntimeValue *items =
          gcrealloc(arr.as.array.items, sizeof(RuntimeValue) * newLen);
      if (!items && newLen > 0) {
        if (error)
          addError(error,
                   (ErrorInfo){.code = "InternalError",
                               .message = "out of memory growing array",
                               .line = 0,
                               .row = 0,
                               .type = ERR_INTERNAL});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      items[newLen - 1] = arg.value;
      arr.as.array.items = items;
      arr.as.array.length = newLen;
    }
    ret = resultNormal(arr);
  } else {
    /* pop(): no arguments allowed; empty array yields null. */
    if (ast->call.length > 0) {
      if (error)
        addError(error,
                 (ErrorInfo){.code = "TypeError",
                             .message = "pop() expects no arguments",
                             .line = 0,
                             .row = 0,
                             .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (arr.as.array.length == 0)
      ret = resultNormal(valueNull());
    else {
      arr.as.array.length--;
      ret = resultNormal(arr.as.array.items[arr.as.array.length]);
    }
  }

  /* Write the mutated array back to its variable. */
  int baseId = calleeAst->member.object;
  if (baseId >= 0 && baseId < node->length) {
    AstNode *baseAst = &node->ast[baseId];
    const char *baseName = NULL;
    if (baseAst->type == NODE_IDENTIFIER)
      baseName = baseAst->identifier.name;
    else if (baseAst->type == NODE_LITERAL_ID)
      baseName = baseAst->string.value;
    if (baseName)
      semSet(env, baseName, arr);
  }

  return ret;
}

InterpreterResult interpretCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  /* Array builtin methods (push/pop) mutate in place — see arrayBuiltinCall. */
  bool handled = false;
  InterpreterResult builtin =
      arrayBuiltinCall(node, ast, env, error, &handled);
  if (handled) return builtin;

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
      if (name && isPrivateName(env, name)) {
        snprintf(message, sizeof(message),
                 "'%s' is private and cannot be called from outside", name);
        addError(error, (ErrorInfo){.code = "PrivateError", .message = message,
                                     .line = 0, .row = 0,
                                     .type = ERR_INVALID_CALL});
      } else if (name)
        snprintf(message, sizeof(message),
                 "'%s' is not a function and cannot be called", name);
      else
        snprintf(message, sizeof(message),
                 "value of type '%s' is not a function and cannot be called",
                 valueTypeName(callee.value.type));
      if (!name || !isPrivateName(env, name))
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
