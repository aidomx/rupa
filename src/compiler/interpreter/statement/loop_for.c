#include <rupa.h>

static bool evalForCondition(int value, int bound, const char *op) {
  return !strcmp(op, "<") ? value < bound : value <= bound;
}

/*
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
InterpreterResult interpretForLoop(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error, RuntimeValue *last) {
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
}
