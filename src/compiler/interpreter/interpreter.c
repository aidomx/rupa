#include <rupa.h>

/* Module execution owns the AST dispatcher; this file owns only the public
 * interpreter lifecycle and process-wide runtime setup. */
extern struct EventLoop *g_event_loop;

void interpreter(Node *node, Error *error) {
  if (!node || node->length <= 0) return;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return;

  stdlibInit(env);
  g_event_loop = eventLoopCreate();

  int root = 0;
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  }

  InterpreterResult result = interpretNode(node, root, env, error);

  /* Run event loop after all top-level statements */
  eventLoopRun(node, g_event_loop, env, error);

  if (result.flow == FLOW_ERROR || (error && error->size > 0)) printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;
  stdlibLoaderCleanup();
}
