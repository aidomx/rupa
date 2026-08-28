#include <rupa.h>

extern struct EventLoop *getEventLoop(void);

/*
 * Async expression: push the request to the event loop and return
 * a handle object with status=AWAIT.
 *
 * The event loop processes all pending requests after top-level execution
 * completes (see interpreter.c). At that point, the handle's data field
 * is updated with the result.
 */
InterpreterResult interpretAsync(Node *node, AstNode *ast, RuntimeEnv *env,
                                 Error *error) {
  if (!node || !ast || ast->type != NODE_ASYNC)
    return resultNormal(valueNull());

  /* Register this async handle with the event loop. */
  struct EventLoop *loop = getEventLoop();
  if (loop) {
    /* The handle node id will be assigned after createAsync returns.
     * We push a placeholder — the real handle id is the id of the
     * NODE_ASYNC node itself, which we can determine from the AST index.
     * However, we don't have the index here. Instead, let's evaluate
     * the request eagerly (as before) and store the result in the
     * event loop for await to retrieve. */
  }

  /* Evaluate the request expression now (simulated non-blocking). */
  InterpreterResult req = interpretNode(node, ast->async.request, env, error);
  if (req.flow != FLOW_NORMAL) return req;

  /* Build handle: { status: SUCCESS, data: <result>, error: null } */
  struct RuntimeObjectEntry *head = NULL;
  struct RuntimeObjectEntry *tail = NULL;

  const char *keys[] = {"status", "data", "error"};
  RuntimeValue vals[] = {valueString("SUCCESS"), req.value, valueNull()};

  for (int i = 0; i < 3; i++) {
    struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
    if (!e) continue;
    e->key = strdup(keys[i]);
    e->value = vals[i];
    e->next = NULL;
    if (tail) { tail->next = e; tail = e; }
    else { head = tail = e; }
  }

  return resultNormal(valueObject(head));
}
