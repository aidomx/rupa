#include <rupa.h>

extern struct EventLoop *getEventLoop(void);

/* Await: return the resolved handle value.
 *
 * Since async evaluates eagerly (AWAIT → request → SUCCESS),
 * the handle is already resolved. Await simply passes it through.
 */
InterpreterResult interpretAwait(Node *node, AstNode *ast, RuntimeEnv *env,
                                 Error *error) {
  if (!node || !ast || ast->type != NODE_AWAIT)
    return resultNormal(valueNull());

  InterpreterResult inner = interpretNode(node, ast->await.expression, env, error);
  if (inner.flow != FLOW_NORMAL) return inner;

  /* If handle is still PENDING, run event loop until done */
  if (inner.value.type == VALUE_OBJECT) {
    RuntimeValue status;
    if (valueObjectGet(inner.value, "status", &status) &&
        status.type == VALUE_STRING &&
        strcmp(status.as.string, "PENDING") == 0) {
      struct EventLoop *loop = getEventLoop();
      if (loop) {
        for (int i = 0; i < 1000; i++) {
          bool allDone = true;
          for (struct AsyncEvent *e = loop->head; e; e = e->next) {
            if (e->state != ASYNC_DONE) { allDone = false; break; }
          }
          if (allDone) break;
          eventLoopRun(node, loop, env, error);
        }
      }
    }
  }

  return resultNormal(inner.value);
}
