#include <rupa.h>

/* Global event loop for async operations. Single-threaded interpreter. */
static struct EventLoop *g_event_loop = NULL;

struct EventLoop *getEventLoop(void) { return g_event_loop; }

InterpreterResult interpretNode(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length)
    return resultNormal(valueNull());

  if (n->ast[id].type == NODE_PROGRAM) {
    RuntimeValue last = valueNull();
    for (AstDeclaration *d = n->ast[id].program.declarations; d; d = d->next) {
      InterpreterResult r = interpretNode(n, d->nodeId, e, x);
      last = r.value;
    }
    /* After all top-level statements, run the event loop to resolve
     * any pending async operations. */
    if (g_event_loop)
      eventLoopRun(n, g_event_loop, e, x);
    return resultNormal(last);
  }

  switch (n->ast[id].type) {
  case NODE_IMPORT: {
    /* Handle import statement: import os from rupa */
    /* The module name is in the expression pointed to by module.value */
    int expr_id = n->ast[id].module.value;
    if (expr_id >= 0 && expr_id < n->length) {
      AstNode *expr = &n->ast[expr_id];
      const char *module_name = NULL;
      if (expr->type == NODE_LITERAL_ID)
        module_name = expr->string.value;
      else if (expr->type == NODE_IDENTIFIER)
        module_name = expr->identifier.name;

      if (module_name) {
        RuntimeValue module_val;
        if (stdlibGetModule(module_name, &module_val)) {
          semSet(e, module_name, module_val);
        }
      }
    }
    return resultNormal(valueNull());
  }
  case NODE_ASSIGN:
  case NODE_CONDITIONAL_ASSIGN:
  case NODE_ANNOTATION:
  case NODE_PRINT:
  case NODE_RETURN:
  case NODE_BLOCK:
  case NODE_IF:
  case NODE_BREAK:
  case NODE_CONTINUE:
  case NODE_FUNCTION_DECL:
  case NODE_LOOP:
  case NODE_CASE:
  case NODE_STRUCT_DECL:
  case NODE_MEMBER_ASSIGN:
    return interpretStatement(n, id, e, x);
  default:
    return interpretExpression(n, id, e, x);
  }
}

void interpreter(Node *node, Error *error) {
  if (!node || node->length <= 0)
    return;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env)
    return;

  /* Initialize standard library modules */
  stdlibInit(env);

  g_event_loop = eventLoopCreate();

  int root = 0;
  for (int i = 0; i < node->length; i++)
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  InterpreterResult result = interpretNode(node, root, env, error);
  if (result.flow == FLOW_ERROR || (error && error->size > 0))
    printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;
}
