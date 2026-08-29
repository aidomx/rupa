#include <rupa.h>

static bool evalRevCondition(int value, int bound, const char *op) {
  return !strcmp(op, ">") ? value > bound : value >= bound;
}

/*
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
InterpreterResult interpretRevLoop(Node *node, AstNode *ast, RuntimeEnv *env,
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
