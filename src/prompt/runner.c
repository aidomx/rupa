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

  /* File mode: bukan REPL. isRepl=false membuat lexer memperlakukan
   * newline dalam block sebagai boundary statement (bare `return` di
   * akhir body sah), dan print() tidak menambah newline REPL. */
  State *state = createGlobalState(length, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return 1;
  }

  const char *index = paths[length];
  /**
   * Menjalankan rupa <file.rp> dengan IR
   * Atur nilai runWithIR=false jika IR sedang dalam masa
   * perbaikan dan gunakan command :
   *  rupa --test-ir <file.rp>
   *  rupa --test-irexec <file.rp>
   *
   * Karena jika tetap menggunakam rupa <file.rp>
   * program akan selalu menghasilkan dari interpreter
   * bukan dari sistem IR.
   *
   * Noted: perkembangan IR dan Interpreter harus sesuai,
   * karena itu sangat menguntungkan jika interpreter-v1.0 tetapi IR-v1.1 artinya IR dalam masa perkembangan.
   *
   * dan user tidak terdampak oleh perkembangan karena secara default akan menggunakan IR-v1.0.
   */
  bool runWithIR = true;

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
  analyzerReset(); /* registry struct per-file */

  Buffer *buffer = state->buffer;
  if (!readfile(index, buffer)) {
    char *path = gcdup(index);
    getcwd(path, MAX_PATH_LENGTH);
    fprintf(stderr, "Message: Cannot read file %s/%s\n", path, index);
    return 1;
  }

  setSourceFilePath(index);
  addToHistory(state);
  addToInput(state);
  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "LexerError", "incomplete or invalid input",
                       state->input->content, state->input->cursor, ERR_UNEXPECTED_EOF);
    printErrors(state->error);
    return 1;
  }

  Request request = createRequestWithError(tokens, 10, state->error);
  Node *node = processGenerate(&request);
  Error *error = createError(10);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "ParserError", "no AST declarations produced",
                       state->input->content, state->input->cursor, ERR_SYNTAX);
    printErrors(state->error);
    return 1;
  }

  int root = -1;
  for (int j = 0; j < node->length; j++) {
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  }
  if (root < 0) {
    addSourceErrorAt(state->error, "ParserError", "program root not found", state->input->content,
                     state->input->cursor, ERR_SYNTAX);
    printErrors(state->error);
    return 1;
  }

  if (runWithIR) {
    IRModule *ir = createIR();
    if (!ir || !rewrite(node, -1, ir)) {
      addSourceErrorAt(state->error, "IRError", "rewrite ast to ir is failed.",
                       state->input->content, state->input->cursor, ERR_SYNTAX);
      printErrors(state->error);
      return 1;
    }

    /* executeIRError memakai Error eksternal sehingga status runtime
     * (TypeError dari IR_CHECK, dll.) terlihat oleh exit code. */
    int status = executeIRError(ir, node, error);
    irModuleFree(ir);

    if (status != 0) printErrors(error);
    return status;
  }

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
  analyzerReset(); /* registry struct per-file */

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
