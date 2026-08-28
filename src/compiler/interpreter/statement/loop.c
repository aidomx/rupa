#include <rupa.h>

static const char *identName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length)
    return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER)
    return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID)
    return ast->string.value;
  return NULL;
}

static bool isRangeLoop(const AstNode *ast) {
  return ast && ast->loop.kind &&
         (!strcmp(ast->loop.kind, "for") || !strcmp(ast->loop.kind, "rev"));
}

static bool rangeOperator(const char *kind, const char *op) {
  if (!kind || !op)
    return false;
  if (!strcmp(kind, "for"))
    return !strcmp(op, "<") || !strcmp(op, "<=");
  return !strcmp(op, ">") || !strcmp(op, ">=");
}

static bool evalForCondition(int value, int bound, const char *op) {
  return !strcmp(op, "<") ? value < bound : value <= bound;
}

static bool evalRevCondition(int value, int bound, const char *op) {
  return !strcmp(op, ">") ? value > bound : value >= bound;
} /*
   * interpretForLoop — handles the `for` range loop.
   *
   * `for` always moves upward (counter starts at 0 or from the current value
   * for explicit conditions).  Only `<` and `<=` operators are accepted.
   *
   * Shorthand:
   *   i = 10; for i  → copy 10, i = 0, run 0..9
   *
   * Explicit condition (identifier on left):
   *   i = 0;  for i < 10  → uses current i, run until bound
   *
   * Explicit condition (identifier on right):
   *   for 10 < i  → uses 10 as bound, run 0..9
   */
static InterpreterResult interpretForLoop(Node *node, AstNode *ast,
                                          RuntimeEnv *env, Error *error,
                                          RuntimeValue *last) {
  AstNode *condition = &node->ast[ast->loop.condition];
  const char *name = NULL;
  RuntimeValue current = valueNull();
  int bound = 0;
  int value = 0;
  const char *op = NULL;
  bool range = false;

  if (condition->type == NODE_IDENTIFIER ||
      condition->type == NODE_LITERAL_ID) {
    /* Shorthand: `for i` — use value as bound, start counter from 0. */
    name = identName(node, ast->loop.condition);
    if (name && semGet(env, name, &current) && current.type == VALUE_NUMBER) {
      bound = current.as.number;
      value = 0;
      semSet(env, name, valueNumber(value));
      range = true;
    }
  } else if (condition->type == NODE_BINARY) {
    op = condition->binary.op;
    /* Try identifier on left first: `for i < 10`. */
    name = identName(node, condition->binary.left);
    if (name && rangeOperator(ast->loop.kind, op)) {
      InterpreterResult r =
          interpretNode(node, condition->binary.right, env, error);
      if (r.flow != FLOW_NORMAL)
        return r;
      if (r.value.type == VALUE_NUMBER) {
        bound = r.value.as.number;
        if (semGet(env, name, &current) && current.type == VALUE_NUMBER)
          value = current.as.number == -1 ? bound : current.as.number;
        else
          value = 0;
        semSet(env, name, valueNumber(value));
        range = true;
      }
    }
    /* Fallback: identifier on right: `for 10 < i`. */
    if (!range) {
      name = identName(node, condition->binary.right);
      if (name && rangeOperator(ast->loop.kind, op)) {
        InterpreterResult r =
            interpretNode(node, condition->binary.left, env, error);
        if (r.flow != FLOW_NORMAL)
          return r;
        if (r.value.type == VALUE_NUMBER) {
          bound = r.value.as.number;
          if (semGet(env, name, &current) && current.type == VALUE_NUMBER)
            value = current.as.number;
          else
            value = 0;
          semSet(env, name, valueNumber(value));
          range = true;
        }
      }
    }
  }

  if (range) {
    for (;;) {
      bool valid = op ? evalForCondition(value, bound, op) : (value < bound);
      if (!valid)
        break;

      semSet(env, name, valueNumber(value));
      InterpreterResult r = interpretNode(node, ast->loop.body, env, error);
      *last = r.value;

      if (r.flow == FLOW_RETURN || r.flow == FLOW_ERROR)
        return r;
      if (r.flow == FLOW_BREAK)
        break;

      value++;
    }

    // Setelah for 0..9
    // Nilai originalnya menjadi 10
    semSet(env, name, valueNumber(bound));
    return resultNormal(*last);
  }

  return resultNormal(valueNull());
} /*
   * interpretRevLoop — handles the `rev` range loop.
   *
   * `rev` always moves downward.  Only `>` and `>=` operators are accepted.
   * * Shorthand:
   *   i = 10; rev i  → i decrements first, run 9..0
   *
   * Explicit condition (identifier on left):
   *   i = 10; rev i > 0  → i decrements first, run 9..1
   *
   * Explicit condition (identifier on right):
   *   rev 10 > i  → i decrements first from 10, run 9..1
   */
static InterpreterResult interpretRevLoop(Node *node, AstNode *ast,
                                          RuntimeEnv *env, Error *error,
                                          RuntimeValue *last) {
  AstNode *condition = &node->ast[ast->loop.condition];
  const char *name = NULL;
  RuntimeValue current = valueNull();
  int bound = 0;
  int value = 0;
  const char *op = NULL;
  bool range = false;

  if (condition->type == NODE_IDENTIFIER ||
      condition->type == NODE_LITERAL_ID) {
    /* Shorthand: `rev i` — use value as bound, start from bound. */
    name = identName(node, ast->loop.condition);
    if (name && semGet(env, name, &current) && current.type == VALUE_NUMBER) {
      bound = current.as.number;
      value = bound;
      semSet(env, name, valueNumber(value));
      range = true;
    }
  } else if (condition->type == NODE_BINARY) {
    op = condition->binary.op;
    /* Try identifier on left first: `rev i > 0`. */
    name = identName(node, condition->binary.left);
    if (name && rangeOperator(ast->loop.kind, op)) {
      InterpreterResult r =
          interpretNode(node, condition->binary.right, env, error);
      if (r.flow != FLOW_NORMAL)
        return r;
      if (r.value.type == VALUE_NUMBER) {
        bound = r.value.as.number;
        if (semGet(env, name, &current) && current.type == VALUE_NUMBER)
          value = current.as.number;
        else
          value = bound;
        range = true;
      }
    }
    /* Fallback: identifier on right: `rev 10 > i`.
     *
     * The literal on the left is the starting upper bound.  The operator
     * determines the floor: `>` → stop at 0 (exclusive), `>=` → stop at
     * -1 (inclusive of 0).  We set bound = 0 so evalRevCondition works
     * correctly, and start the counter at (left_value - 1). */
    if (!range) {
      name = identName(node, condition->binary.right);
      if (name && rangeOperator(ast->loop.kind, op)) {
        InterpreterResult r =
            interpretNode(node, condition->binary.left, env, error);
        if (r.flow != FLOW_NORMAL)
          return r;
        if (r.value.type == VALUE_NUMBER) {
          bound = 0;
          value = r.value.as.number;
          semSet(env, name, valueNumber(value));
          range = true;
        }
      }
    }
  }

  if (range) {
    for (;;) {
      bool valid = op ? evalRevCondition(value, bound, op) : (value >= 0);
      if (!valid)
        break;

      semSet(env, name, valueNumber(value));
      InterpreterResult r = interpretNode(node, ast->loop.body, env, error);
      *last = r.value;

      if (r.flow == FLOW_RETURN || r.flow == FLOW_ERROR)
        return r;
      if (r.flow == FLOW_BREAK)
        break;

      value--;
    }

    // Setelah rev 10..0
    // Nilai original menjadi -1
    semSet(env, name, valueNumber(value));
    return resultNormal(*last);
  }

  return resultNormal(valueNull());
}

InterpreterResult interpretLoop(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error) {
  if (!node || !ast || ast->type != NODE_LOOP)
    return resultNormal(valueNull());

  RuntimeValue last = valueNull();

  /* Dispatch to the appropriate range-loop handler. */
  if (isRangeLoop(ast) && ast->loop.condition >= 0) {
    if (!strcmp(ast->loop.kind, "for"))
      return interpretForLoop(node, ast, env, error, &last);
    if (!strcmp(ast->loop.kind, "rev"))
      return interpretRevLoop(node, ast, env, error, &last);
  }

  /* while / unconditional loop (generic path). */
  const long maxIterations = 10000000L;
  long iterations = 0;
  for (;;) {
    if (ast->loop.condition >= 0) {
      InterpreterResult cond =
          interpretNode(node, ast->loop.condition, env, error);
      if (cond.flow != FLOW_NORMAL)
        return cond;
      if (!valueTruthy(cond.value))
        break;
    }
    if (++iterations > maxIterations) {
      if (error)
        addError(error,
                 (ErrorInfo){.code = "RuntimeError",
                             .message = "loop exceeded maximum iteration limit",
                             .line = 0,
                             .row = 0,
                             .type = ERR_STACK_OVERFLOW});
      return resultFlow(FLOW_ERROR, last);
    }
    InterpreterResult r = interpretNode(node, ast->loop.body, env, error);
    last = r.value;
    if (r.flow == FLOW_BREAK)
      break;
    if (r.flow == FLOW_CONTINUE)
      continue;
    if (r.flow == FLOW_RETURN || r.flow == FLOW_ERROR)
      return r;
    if (ast->loop.condition < 0)
      break;
  }
  return resultNormal(last);
}
