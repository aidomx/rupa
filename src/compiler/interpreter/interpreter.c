#include <rupa.h>

/* Module execution owns the AST dispatcher; this file owns only the public
 * interpreter lifecycle and process-wide runtime setup. */
extern struct EventLoop *g_event_loop;

/* Statement top-level berupa call `main()` tanpa argumen? Dipakai
 * untuk skip auto-run bila main sudah dipanggil eksplisit. */
static bool topLevelIsMainCall(Node *node, int id) {
  if (id < 0 || id >= node->length) return false;
  AstNode *a = &node->ast[id];
  if (a->type != NODE_CALL || a->call.length != 0) return false;
  int calleeId = a->call.callee;
  if (calleeId < 0 || calleeId >= node->length) return false;
  AstNode *c = &node->ast[calleeId];
  const char *name = NULL;
  if (c->type == NODE_IDENTIFIER)
    name = c->identifier.name;
  else if (c->type == NODE_LITERAL_ID)
    name = c->string.value;
  return name && !strcmp(name, "main");
}

/* `main()` otomatis (design class, entry point): bila ada binding main
 * = fungsi zero-param dan top-level tidak memanggilnya eksplisit,
 * jalankan body setelah seluruh statement top-level — sejajar jalur
 * IR (runUserMain di ir/execute.c). */
static void runUserMain(Node *node, int root, RuntimeEnv *env, Error *error) {
  RuntimeValue fn = valueNull();
  if (!semGet(env, "main", &fn) || fn.type != VALUE_FUNCTION || !fn.as.function) return;
  if (fn.as.function->paramLength != 0) return;

  for (AstDeclaration *d = node->ast[root].program.declarations; d; d = d->next)
    if (topLevelIsMainCall(node, d->nodeId)) return; /* sudah dipanggil eksplisit */

  RuntimeFunction *f = fn.as.function;
  RuntimeEnv *local = semCreateEnv(f->closure ? f->closure : env);
  if (!local) return;
  (void)interpretNode(f->node, f->body, local, error);
}

void interpreter(Node *node, Error *error) {
  if (!node || node->length <= 0) return;

  analyzerReset(); /* registry struct per-file, tidak bocor lintas program */
  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return;

  stdlibInit(env);
  builtinsInit(env);
  g_event_loop = eventLoopCreate();

  int root = 0;
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  }

  InterpreterResult result = interpretNode(node, root, env, error);

  /* main() otomatis — konvensi entry point, tanpa call eksplisit. */
  runUserMain(node, root, env, error);

  /* Run event loop after all top-level statements */
  eventLoopRun(node, g_event_loop, env, error);

  if (result.flow == FLOW_ERROR || (error && error->size > 0)) printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;
  stdlibLoaderCleanup();
}
