#include <rupa.h>
/* Sub-handlers implemented in loop_for.c and loop_rev.c. */
InterpreterResult interpretForLoop(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error, RuntimeValue *last);
InterpreterResult interpretRevLoop(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error, RuntimeValue *last);

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
