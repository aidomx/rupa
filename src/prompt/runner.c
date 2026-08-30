#include <rupa.h>

void run(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No such file for execute.\n");
    return;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return;
  }

  const char *index = paths[length];
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->repl->buffer;

  if (!readfile(index, buffer)) {
    fprintf(stderr, "Cannot read file: %s\n", index);
    return;
  }

  /* Set source file path for resolving relative imports */
  setSourceFilePath(index);

  /* Lex */
  addToHistory(state);
  addToInput(state);
  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;

  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
    return;
  }

  /* Parse */
  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);
  Error *error = createError(10);

  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    return;
  }

  /* Find program root */
  int root = -1;
  for (int j = 0; j < node->length; j++) {
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  }
  if (root < 0) return;

  /* Interpret — clean output, no debug */
  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return;
  stdlibInit(env);

  InterpreterResult result = interpretNode(node, root, env, error);
  if (error && error->size > 0) printErrors(error);
  (void)result;
}

void execute(const char *code) {
  if (!code || strlen(code) == 0) {
    fprintf(stderr, "No code provided.\n");
    return;
  }

  State *state = createGlobalState(10, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return;
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->repl->buffer;
  size_t len = strlen(code);
  if ((int)len >= buffer->capacity) {
    fprintf(stderr, "Code is too long.\n");
    return;
  }

  memcpy(buffer->value, code, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  processInput(state);

  Flags *flags = state->input->flags;
  if (!state->tokens || state->tokens->length == 0 ||
      !hasAstDeclarations(state->tokens) || (flags && flags->isWaiting)) {
    fprintf(stderr, "Execution failed.\n");
  }
}
