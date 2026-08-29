#include <rupa.h>

static const char *nameOf(Node *n, int id) {
  return n && id >= 0 && id < n->length && n->ast[id].type == NODE_IDENTIFIER
             ? n->ast[id].identifier.name
             : NULL;
}

static const char *typeOf(Node *n, int id) {
  if (!n || id < 0 || id >= n->length) return NULL;
  AstNode *a = &n->ast[id];
  if (a->type == NODE_IDENTIFIER) return a->identifier.name;
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  return NULL;
}

InterpreterResult interpretStatement(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length)
    return resultNormal(valueNull());

  AstNode *a = &n->ast[id];

  switch (a->type) {
  case NODE_FUNCTION_DECL:
    return interpretFunction(n, a, e, x);
  case NODE_ASSIGN: {
    const char *k = nameOf(n, a->assign.target);
    InterpreterResult r = interpretNode(n, a->assign.value, e, x);
    if (r.flow != FLOW_NORMAL)
      return r;
    if (a->assign.type >= 0 &&
        !validateAnnotation(n, a->assign.type, r.value, x))
      return resultFlow(FLOW_ERROR, valueNull());
    if (k) {
      const char *type = semType(e, k);
      if (type && !validateTypeName(type, r.value, x))
        return resultFlow(FLOW_ERROR, valueNull());
      semSet(e, k, r.value);
    }
    return r;
  }
  case NODE_CONDITIONAL_ASSIGN: {
    const char *k = nameOf(n, a->conditionalAssign.target);
    RuntimeValue old;
    if (k && semGet(e, k, &old) && valueTruthy(old))
      return resultNormal(old);
    InterpreterResult r = interpretNode(n, a->conditionalAssign.value, e, x);
    if (k)
      semSet(e, k, r.value);
    return r;
  }
  case NODE_ANNOTATION: {
    const char *k = nameOf(n, a->annotation.name);
    const char *type = typeOf(n, a->annotation.type);

    if (a->annotation.value < 0) {
      if (k) semDeclare(e, k, type);
      return resultNormal(valueNull());
    }

    InterpreterResult r = interpretNode(n, a->annotation.value, e, x);
    if (r.flow != FLOW_NORMAL) return r;
    if (!validateAnnotation(n, a->annotation.type, r.value, x))
      return resultFlow(FLOW_ERROR, valueNull());
    if (k) {
      semDeclare(e, k, type);
      semSet(e, k, r.value);
    }
    return r;
  }
  case NODE_PRINT: {
    RuntimeValue last = valueNull();

    for (int i = 0; i < a->print.length; i++) {
      InterpreterResult result = interpretNode(n, a->print.args[i], e, x);
      last = result.value;

      if (result.flow != FLOW_NORMAL)
        return result;

      valuePrintInterp(last, e);

      if (i + 1 < a->print.length)
        putchar(' ');
    }

    putchar('\n');
    return resultNormal(last);
  }
  case NODE_RETURN:
    return resultFlow(FLOW_RETURN,
                      interpretNode(n, a->asReturn.expression, e, x).value);
  case NODE_BLOCK: {
    RuntimeValue last = valueNull();
    for (int i = 0; i < a->block.length; i++) {
      InterpreterResult r = interpretNode(n, a->block.statements[i], e, x);
      last = r.value;
      if (r.flow != FLOW_NORMAL)
        return r;
    }
    return resultNormal(last);
  }
  case NODE_IF: {
    InterpreterResult c = interpretNode(n, a->asIf.condition, e, x);
    if (c.flow != FLOW_NORMAL)
      return c;
    if (valueTruthy(c.value))
      return interpretNode(n, a->asIf.thenBlock, e, x);
    if (a->asIf.elseBlock >= 0)
      return interpretNode(n, a->asIf.elseBlock, e, x);
    return resultNormal(valueNull());
  }
  case NODE_BREAK:
    return resultFlow(FLOW_BREAK, valueNull());
  case NODE_CONTINUE:
    return resultFlow(FLOW_CONTINUE, valueNull());
  case NODE_LOOP:
    return interpretLoop(n, a, e, x);
  case NODE_CASE:
    return interpretCase(n, a, e, x);
  case NODE_STRUCT_DECL:
    return interpretStruct(n, a, e, x);
  case NODE_MEMBER_ASSIGN:
    return interpretMemberAssign(n, a, e, x);
  default:
    return interpretExpression(n, id, e, x);
  }
}
