#include <rupa.h>

static const char *nameOf(Node *n, int id) {
  return n && id >= 0 && id < n->length && n->ast[id].type == NODE_IDENTIFIER
             ? n->ast[id].identifier.name
             : NULL;
}

static bool formatType(Node *n, int id, char *buffer, size_t capacity) {
  if (!n || id < 0 || id >= n->length || !buffer || capacity == 0) return false;

  AstNode *a = &n->ast[id];
  if (a->type == NODE_IDENTIFIER) {
    int written = snprintf(buffer, capacity, "%s", a->identifier.name);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type == NODE_LITERAL_ID) {
    int written = snprintf(buffer, capacity, "%s", a->string.value);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type != NODE_ARRAY_TYPE) return false;

  char element[256];
  if (!formatType(n, a->arrayType.elementType, element, sizeof(element))) return false;
  int written = snprintf(buffer, capacity, "%s[]", element);
  return written > 0 && (size_t)written < capacity;
}

static const char *typeOf(Node *n, int id) {
  static char typeName[256];
  return formatType(n, id, typeName, sizeof(typeName)) ? typeName : NULL;
}

InterpreterResult interpretStatement(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length) return resultNormal(valueNull());

  AstNode *a = &n->ast[id];

  switch (a->type) {
  case NODE_FUNCTION_DECL:
    return interpretFunction(n, a, e, x);
  case NODE_ASSIGN: {
    const char *k = nameOf(n, a->assign.target);
    InterpreterResult r = interpretNode(n, a->assign.value, e, x);
    if (r.flow != FLOW_NORMAL) return r;
    if (a->assign.type >= 0 && !validateAnnotation(n, a->assign.type, r.value, x))
      return resultFlow(FLOW_ERROR, valueNull());
    if (k) {
      /* Preserve the explicit type on the binding so later assignments are
       * checked too (`x: number[] = []; x = [1]`). */
      const char *declaredType = a->assign.type >= 0 ? typeOf(n, a->assign.type) : NULL;
      if (declaredType) semDeclare(e, k, declaredType);

      const char *type = semType(e, k);
      if (type && !validateTypeName(type, r.value, x)) return resultFlow(FLOW_ERROR, valueNull());
      semSet(e, k, r.value);
    }
    return r;
  }
  case NODE_CONDITIONAL_ASSIGN: {
    const char *k = nameOf(n, a->conditionalAssign.target);
    RuntimeValue old;
    if (k && semGet(e, k, &old) && valueTruthy(old)) return resultNormal(old);
    InterpreterResult r = interpretNode(n, a->conditionalAssign.value, e, x);
    if (k) semSet(e, k, r.value);
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

      if (result.flow != FLOW_NORMAL) return result;

      valuePrintInterp(last, e, x);

      if (i + 1 < a->print.length) putchar(' ');
    }

    if (e->isRepl) {
      putchar('\n');
      fflush(stdout);
    }

    return resultNormal(last);
  }
  case NODE_RETURN:
    return resultFlow(FLOW_RETURN, interpretNode(n, a->asReturn.expression, e, x).value);
  case NODE_BLOCK: {
    RuntimeValue last = valueNull();
    for (int i = 0; i < a->block.length; i++) {
      InterpreterResult r = interpretNode(n, a->block.statements[i], e, x);
      last = r.value;
      if (r.flow != FLOW_NORMAL) return r;
    }
    return resultNormal(last);
  }
  case NODE_IF: {
    InterpreterResult c = interpretNode(n, a->asIf.condition, e, x);
    if (c.flow != FLOW_NORMAL) return c;
    if (valueTruthy(c.value)) return interpretNode(n, a->asIf.thenBlock, e, x);
    if (a->asIf.elseBlock >= 0) return interpretNode(n, a->asIf.elseBlock, e, x);
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
