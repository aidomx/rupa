#include <rupa.h>

static const char *memberName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  return NULL;
}

InterpreterResult interpretMember(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  if (!node || !ast || ast->type != NODE_MEMBER)
    return resultNormal(valueNull());

  InterpreterResult obj = interpretNode(node, ast->member.object, env, error);
  if (obj.flow != FLOW_NORMAL) return obj;

  const char *key = memberName(node, ast->member.member);

  if (obj.value.type == VALUE_OBJECT) {
    RuntimeValue val;
    if (valueObjectGet(obj.value, key, &val))
      return resultNormal(val);
    return resultNormal(valueNull());
  }

  /* Array .length property */
  if (obj.value.type == VALUE_ARRAY && key && !strcmp(key, "length"))
    return resultNormal(valueNumber(obj.value.as.array.length));

  /* String .length property and bound methods */
  if (obj.value.type == VALUE_STRING && key) {
    if (!strcmp(key, "length"))
      return resultNormal(valueNumber(obj.value.as.string
                                          ? (int)strlen(obj.value.as.string)
                                          : 0));

    NativeFn fn = NULL;
    int paramCount = 0;
    if (!strcmp(key, "upper")) {
      fn = stdStringUpper;
      paramCount = 0;
    } else if (!strcmp(key, "lower")) {
      fn = stdStringLower;
      paramCount = 0;
    } else if (!strcmp(key, "trim")) {
      fn = stdStringTrim;
      paramCount = 0;
    } else if (!strcmp(key, "contains")) {
      fn = stdStringContains;
      paramCount = 1;
    } else if (!strcmp(key, "startsWith")) {
      fn = stdStringStartsWith;
      paramCount = 1;
    } else if (!strcmp(key, "endsWith")) {
      fn = stdStringEndsWith;
      paramCount = 1;
    } else if (!strcmp(key, "replace")) {
      fn = stdStringReplace;
      paramCount = 2;
    }
    if (fn) {
      RuntimeValue method = valueNativeFunction(key, fn, paramCount);
      method.as.nativeFunc->hasReceiver = true;
      method.as.nativeFunc->receiver = malloc(sizeof(RuntimeValue));
      if (method.as.nativeFunc->receiver)
        *method.as.nativeFunc->receiver = obj.value;
      return resultNormal(method);
    }
  }

  if (obj.value.type == VALUE_NULL)
    return resultNormal(valueNull());

  static char message[256];
  snprintf(message, sizeof(message),
           "cannot access property '%s' on value of type '%s'",
           key ? key : "?", valueTypeName(obj.value.type));
  if (error)
    addError(error, (ErrorInfo){.code = "TypeError",
                                .message = message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_TYPE_MISMATCH});
  return resultFlow(FLOW_ERROR, valueNull());
}

/*
 * Member assignment: obj.field = value or arr[i] = value.
 *
 * The target AST node is either NODE_MEMBER or NODE_SUBSCRIPT.
 * For objects: update the entry in-place via valueObjectSet.
 * For arrays:  replace the element at the given index.
 */
InterpreterResult interpretMemberAssign(Node *node, AstNode *ast,
                                        RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_MEMBER_ASSIGN)
    return resultNormal(valueNull());

  AstNode *target = &node->ast[ast->memberAssign.target];

  /* Evaluate the value to assign. */
  InterpreterResult val =
      interpretNode(node, ast->memberAssign.value, env, error);
  if (val.flow != FLOW_NORMAL) return val;

  /* --- Object member assignment: obj.field = value --- */
  if (target->type == NODE_MEMBER) {
    /* Resolve the base object (not the leaf member). */
    InterpreterResult base =
        interpretNode(node, target->member.object, env, error);
    if (base.flow != FLOW_NORMAL) return base;

    const char *key = memberName(node, target->member.member);

    if (base.value.type == VALUE_OBJECT) {
      if (!valueObjectSet(&base.value, key, val.value)) {
        if (error)
          addError(error,
                   (ErrorInfo){.code = "InternalError",
                               .message = "failed to set object property",
                               .line = 0,
                               .row = 0,
                               .type = ERR_INTERNAL});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      /* Write the mutated object back to the environment so the
       * assignment is visible to subsequent lookups. */
      if (target->member.object >= 0 &&
          target->member.object < node->length) {
        AstNode *baseAst = &node->ast[target->member.object];
        const char *baseName = NULL;
        if (baseAst->type == NODE_IDENTIFIER)
          baseName = baseAst->identifier.name;
        else if (baseAst->type == NODE_LITERAL_ID)
          baseName = baseAst->string.value;
        if (baseName)
          semSet(env, baseName, base.value);
      }
      return resultNormal(val.value);
    }

    static char message[256];
    snprintf(message, sizeof(message),
             "cannot assign property '%s' on value of type '%s'",
             key ? key : "?", valueTypeName(base.value.type));
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  /* --- Array element assignment: arr[i] = value --- */
  if (target->type == NODE_SUBSCRIPT) {
    InterpreterResult base =
        interpretNode(node, target->subscript.posId, env, error);
    if (base.flow != FLOW_NORMAL) return base;

    if (base.value.type != VALUE_ARRAY) {
      static char message[256];
      snprintf(message, sizeof(message),
               "cannot index into value of type '%s'",
               valueTypeName(base.value.type));
      if (error)
        addError(error,
                 (ErrorInfo){.code = "TypeError",
                             .message = message,
                             .line = 0,
                             .row = 0,
                             .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    InterpreterResult idx =
        interpretNode(node, target->subscript.index, env, error);
    if (idx.flow != FLOW_NORMAL) return idx;

    if (idx.value.type != VALUE_NUMBER) {
      if (error)
        addError(error,
                 (ErrorInfo){.code = "TypeError",
                             .message = "array index must be a number",
                             .line = 0,
                             .row = 0,
                             .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    int i = idx.value.as.number;
    int len = base.value.as.array.length;
    if (i < 0 || i >= len) {
      static char message[256];
      snprintf(message, sizeof(message),
               "index %d is out of bounds for array of length %d", i, len);
      if (error)
        addError(error,
                 (ErrorInfo){.code = "RangeError",
                             .message = message,
                             .line = 0,
                             .row = 0,
                             .type = ERR_INDEX_OUT_OF_BOUNDS});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    base.value.as.array.items[i] = val.value;

    /* Write the mutated array back. */
    if (target->subscript.posId >= 0 &&
        target->subscript.posId < node->length) {
      AstNode *baseAst = &node->ast[target->subscript.posId];
      const char *baseName = NULL;
      if (baseAst->type == NODE_IDENTIFIER)
        baseName = baseAst->identifier.name;
      else if (baseAst->type == NODE_LITERAL_ID)
        baseName = baseAst->string.value;
      if (baseName)
        semSet(env, baseName, base.value);
    }
    return resultNormal(val.value);
  }

  if (error)
    addError(error,
             (ErrorInfo){.code = "SyntaxError",
                         .message = "invalid assignment target",
                         .line = 0,
                         .row = 0,
                         .type = ERR_SYNTAX});
  return resultFlow(FLOW_ERROR, valueNull());
}
