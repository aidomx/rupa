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

  /* old_loop/mixed (design/next_loop.txt): `for i=0; i < 10: ...` /
   * `rev i=10; i > 0 { ... }` — init dieksekusi SEKALI sebelum loop;
   * increment/decrement tetap diurus sistem (for maju, rev mundur). */
  if (ast->loop.init >= 0) {
    InterpreterResult ir = interpretNode(node, ast->loop.init, env, error);
    if (ir.flow == FLOW_RETURN || ir.flow == FLOW_ERROR)
      return ir;
    if (ir.flow == FLOW_BREAK)
      return resultNormal(ir.value);
    /* FLOW_CONTINUE pada posisi init: perlakukan sebagai skip ke
     * iterasi pertama (tanpa efek). */
  }

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
