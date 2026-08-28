#include <rupa.h>

struct EventLoop *eventLoopCreate(void) {
  struct EventLoop *loop = calloc(1, sizeof(*loop));
  return loop;
}

void eventLoopPush(struct EventLoop *loop, int handleId) {
  if (!loop) return;
  struct AsyncEvent *e = calloc(1, sizeof(*e));
  if (!e) return;
  e->handleId = handleId;
  e->result = valueNull();
  e->done = false;
  e->next = NULL;

  if (loop->tail) {
    loop->tail->next = e;
    loop->tail = e;
  } else {
    loop->head = loop->tail = e;
  }
  loop->count++;
}

void eventLoopRun(Node *node, struct EventLoop *loop, RuntimeEnv *env,
                  Error *error) {
  if (!loop) return;

  for (struct AsyncEvent *e = loop->head; e; e = e->next) {
    if (e->done) continue;

    /* Look up the async handle node to get the request expression. */
    if (e->handleId < 0 || e->handleId >= node->length) continue;
    AstNode *ast = &node->ast[e->handleId];
    if (ast->type != NODE_ASYNC) continue;

    /* Evaluate the request expression. */
    InterpreterResult r = interpretNode(node, ast->async.request, env, error);
    if (r.flow == FLOW_ERROR) continue;

    e->result = r.value;
    e->done = true;
  }
}

bool eventLoopGetResult(struct EventLoop *loop, int handleId,
                        RuntimeValue *out) {
  if (!loop) return false;
  for (struct AsyncEvent *e = loop->head; e; e = e->next)
    if (e->handleId == handleId && e->done) {
      if (out) *out = e->result;
      return true;
    }
  return false;
}

void eventLoopDestroy(struct EventLoop *loop) {
  if (!loop) return;
  struct AsyncEvent *e = loop->head;
  while (e) {
    struct AsyncEvent *next = e->next;
    free(e);
    e = next;
  }
  free(loop);
}
