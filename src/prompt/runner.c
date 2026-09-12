#include <rupa.h>

static int isDirectory(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  return S_ISDIR(st.st_mode);
}

int run(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No such file for execute.\n");
    return 1;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return 1;
  }

  const char *index = paths[length];
  if (isDirectory(index)) {
    static char indexBuf[1024];
    snprintf(indexBuf, sizeof(indexBuf), "%s/index.rp", index);
    index = indexBuf;
  }
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  if (!readfile(index, buffer)) {
    fprintf(stderr, "Cannot read file: %s\n", index);
    return 1;
  }

  setSourceFilePath(index);
  addToHistory(state);
  addToInput(state);
  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) return 1;

  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);
  Error *error = createError(10);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) return 1;

  int root = -1;
  for (int j = 0; j < node->length; j++) {
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  }
  if (root < 0) return 1;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return 1;
  stdlibInit(env);
  builtinsInit(env);

  extern struct EventLoop *g_event_loop;
  g_event_loop = eventLoopCreate();

  InterpreterResult result = interpretNode(node, root, env, error);

  /* Run event loop until all pending events are done */
  for (int i = 0; i < 1000 && eventLoopHasPending(g_event_loop); i++) {
    eventLoopRun(node, g_event_loop, env, error);
  }

  if (error && error->size > 0) printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;
  return result.flow == FLOW_ERROR || (error && error->size > 0) ? 1 : 0;
}

void execute(const char *code) {
  if (!code || strlen(code) == 0) {
    fprintf(stderr, "No code provided.\n");
    return;
  }

  State *state = createGlobalState(10, true);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return;
  }

  state->isRepl = false;
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  size_t len = strlen(code);
  if ((int)len >= buffer->capacity) {
    fprintf(stderr, "Code is too long.\n");
    return;
  }

  memcpy(buffer->value, code, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  processInput(state);
}
