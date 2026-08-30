#include <rupa.h>

/* ================================================================
 * Shared helper: print source lines with line numbers.
 * ================================================================ */
static void printSource(const char *src) {
  int line_no = 1;
  printf("  %3d | ", line_no);
  for (int c = 0; src[c]; c++) {
    putchar(src[c]);
    if (src[c] == '\n' && src[c + 1]) {
      line_no++;
      printf("  %3d | ", line_no);
    }
  }
  if (src[strlen(src) - 1] != '\n') putchar('\n');
}

/* ================================================================
 * Shared helper: run lex+parse on a file, return true if valid.
 * ================================================================ */
static bool lexParse(State *state, const char *path, Buffer **outBuf,
                     Token **outTokens, Node **outNode) {
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->repl->buffer;
  if (!readfile(path, buffer)) {
    *outBuf = buffer;
    *outTokens = NULL;
    *outNode = NULL;
    return false;
  }

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 ||
      (state->input->flags && state->input->flags->isWaiting)) {
    *outBuf = buffer;
    *outTokens = tokens;
    *outNode = NULL;
    return false;
  }

  Request req = createRequest(tokens, 10);
  Node *node = processGenerate(&req);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    *outBuf = buffer;
    *outTokens = tokens;
    *outNode = NULL;
    return false;
  }

  *outBuf = buffer;
  *outTokens = tokens;
  *outNode = node;
  return true;
}

/* ================================================================
 * Syntax test: lex + parse + interpret, show output per file.
 * Same behavior as ./bin/rupa <file>, with PASS/FAIL summary.
 * ================================================================ */

void test(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
    return;
  }

  int passed = 0;
  int failed = 0;

  printf("> Testing Rupa syntax\n");

  for (int i = 0; i < length; i++) {
    clearReplState(state->repl);
    clearInput(state->input);
    clearStateToken(state->tokens);
    clearStateContext(state->context);
    state->size = 0;

    Buffer *buffer = state->repl->buffer;

    if (!readfile(paths[i], buffer)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    addToHistory(state);
    addToInput(state);
    lexer(state);

    Flags *flags = state->input->flags;
    Token *tokens = state->tokens;

    if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    Request request = createRequest(tokens, 10);
    Node *node = processGenerate(&request);
    Error *error = createError(10);

    if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    /* Find program root */
    int root = -1;
    for (int j = 0; j < node->length; j++) {
      if (node->ast[j].type == NODE_PROGRAM) {
        root = j;
        break;
      }
    }
    if (root < 0) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    /* Execute — same as ./bin/rupa <file> */
    setSourceFilePath(paths[i]);
    RuntimeEnv *env = semCreateEnv(NULL);
    if (!env) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }
    stdlibInit(env);

    InterpreterResult result = interpretNode(node, root, env, error);

    bool exec_ok = (result.flow == FLOW_ERROR) ? false : true;
    bool no_errors = (error && error->size == 0);

    if (exec_ok && no_errors) {
      printf("PASS | %s\n", paths[i]);
      passed++;
    } else {
      printf("FAIL | %s\n", paths[i]);
      if (!exec_ok) printf("       execution error\n");
      if (!no_errors) printErrors(error);
      failed++;
    }
  }

  printf("\n> Test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}

/* ================================================================
 * AST test: lex + parse, show source + AST structure.
 * ================================================================ */

void testAst(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
    return;
  }

  int passed = 0;
  int failed = 0;

  printf("> AST Structure\n");

  for (int i = 0; i < length; i++) {
    Buffer *buffer;
    Token *tokens;
    Node *node;

    if (!lexParse(state, paths[i], &buffer, &tokens, &node)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    printf("\n--- %s ---\n", paths[i]);
    printf("Source:\n");
    printSource(buffer->value);
    printf("AST:\n");
    startDebug(node);
    printf("PASS\n");
    passed++;
  }

  printf("\n> AST test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}

/* ================================================================
 * Execution test: parse + interpret, show source + results.
 * ================================================================ */

void testExec(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
    return;
  }

  int passed = 0;
  int failed = 0;

  printf("> Execution tests\n");

  for (int i = 0; i < length; i++) {
    /* Skip repl_*.rp files — they require shared env across lines
     * and must be run with --test-repl, not --test-exec. */
    {
      const char *base = strrchr(paths[i], '/');
      base = base ? base + 1 : paths[i];
      if (strncmp(base, "repl_", 5) == 0) {
        printf("SKIP | %s (use --test-repl)\n", paths[i]);
        continue;
      }
    }

    clearReplState(state->repl);
    clearInput(state->input);
    clearStateToken(state->tokens);
    clearStateContext(state->context);
    state->size = 0;

    Buffer *buffer = state->repl->buffer;

    if (!readfile(paths[i], buffer)) {
      printf("FAIL | %s (file not found)\n", paths[i]);
      failed++;
      continue;
    }

    /* Show program source */
    printf("\n--- %s ---\n", paths[i]);
    printf("Source:\n");
    printSource(buffer->value);
    printf("\n");

    addToHistory(state);
    addToInput(state);
    lexer(state);

    Flags *flags = state->input->flags;
    Token *tokens = state->tokens;

    if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
      printf("FAIL | %s (lex failed)\n", paths[i]);
      failed++;
      continue;
    }

    Request request = createRequest(tokens, 10);
    Node *node = processGenerate(&request);
    Error *error = createError(10);

    if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
      printf("FAIL | %s (parse failed)\n", paths[i]);
      failed++;
      continue;
    }

    testHelperReset();
    setSourceFilePath(paths[i]);
    RuntimeEnv *env = semCreateEnv(NULL);
    if (!env) {
      printf("FAIL | %s (env alloc failed)\n", paths[i]);
      failed++;
      continue;
    }
    stdlibInit(env);
    testHelperInit(env);

    InterpreterResult result = interpretNode(node, 0, env, error);

    bool exec_ok = (result.flow == FLOW_ERROR) ? false : true;
    bool assert_ok = (testHelperFailures() == 0);
    bool no_errors = (error && error->size == 0);

    if (exec_ok && assert_ok && no_errors) {
      printf("PASS | %s\n", paths[i]);
      passed++;
    } else {
      printf("FAIL | %s\n", paths[i]);
      if (!exec_ok)
        printf("       execution error\n");
      if (!assert_ok)
        printf("       %d assertion(s) failed\n", testHelperFailures());
      if (!no_errors)
        printErrors(error);
      failed++;
    }
  }

  printf("\n> Execution test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}

/* ================================================================
 * REPL execution boundary test: simulate multi-line REPL input
 * with shared environment, verify state across lines.
 * ================================================================ */

void testRepl(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  int passed = 0;
  int failed = 0;

  printf("> REPL execution boundary tests\n");

  for (int i = 0; i < length; i++) {
    printf("\n--- %s ---\n", paths[i]);

    FILE *fp = fopen(paths[i], "r");
    if (!fp) {
      printf("FAIL | %s (file not found)\n", paths[i]);
      failed++;
      continue;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = malloc(fsize + 1);
    if (!content) {
      fclose(fp);
      printf("FAIL | %s (alloc failed)\n", paths[i]);
      failed++;
      continue;
    }
    fread(content, 1, fsize, fp);
    content[fsize] = '\0';
    fclose(fp);

    State *state = createGlobalState(10, true);
    if (!state || !state->repl || !state->repl->buffer) {
      free(content);
      printf("FAIL | %s (state alloc failed)\n", paths[i]);
      failed++;
      continue;
    }

    RuntimeEnv *sharedEnv = semCreateEnv(NULL);
    if (!sharedEnv) {
      clearGlobalState(state, 10);
      free(content);
      printf("FAIL | %s (env alloc failed)\n", paths[i]);
      failed++;
      continue;
    }
    stdlibInit(sharedEnv);
    testHelperInit(sharedEnv);
    testHelperReset();
    setSourceFilePath(paths[i]);

    bool test_failed = false;
    int line_num = 0;

    Node **all_nodes = NULL;
    int node_count = 0;
    int node_cap = 0;

    char *saveptr = NULL;
    char *line = strtok_r(content, "\n", &saveptr);
    while (line) {
      line_num++;

      while (*line == ' ' || *line == '\t')
        line++;
      if (*line == '\0' || *line == '#') {
        line = strtok_r(NULL, "\n", &saveptr);
        continue;
      }

      if (state->repl->buffer->value)
        state->repl->buffer->value[0] = '\0';
      state->repl->buffer->length = 0;
      state->repl->size = 0;
      clearStateContext(state->context);
      state->size = 0;

      size_t len = strlen(line);
      if ((int)len >= state->repl->buffer->capacity) {
        test_failed = true;
        line = strtok_r(NULL, "\n", &saveptr);
        continue;
      }
      memcpy(state->repl->buffer->value, line, len);
      state->repl->buffer->value[len] = '\0';
      state->repl->buffer->length = (int)len;

      printf("  %2d> %s\n", line_num, line);

      addToHistory(state);
      addToInput(state);
      lexer(state);

      Flags *flags = state->input->flags;
      Token *tokens = state->tokens;

      if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
        line = strtok_r(NULL, "\n", &saveptr);
        continue;
      }

      {
        Request req = createRequest(tokens, 10);
        Node *node = processGenerate(&req);
        Error *error = createError(10);

        if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
          test_failed = true;
          line = strtok_r(NULL, "\n", &saveptr);
          continue;
        }

        if (node_count >= node_cap) {
          node_cap = node_cap ? node_cap * 2 : 16;
          all_nodes = realloc(all_nodes, sizeof(Node *) * node_cap);
        }
        all_nodes[node_count++] = node;

        int root = -1;
        for (int j = 0; j < node->length; j++) {
          if (node->ast[j].type == NODE_PROGRAM) {
            root = j;
            break;
          }
        }
        if (root < 0) {
          test_failed = true;
          line = strtok_r(NULL, "\n", &saveptr);
          continue;
        }

        InterpreterResult result = interpretNode(node, root, sharedEnv, error);

        if (result.flow == FLOW_ERROR || (error && error->size > 0)) {
          printf("  FAIL line %d\n", line_num);
          if (error && error->size > 0) printErrors(error);
          test_failed = true;
          break;
        }
      }

      line = strtok_r(NULL, "\n", &saveptr);
    }

    if (!test_failed && testHelperFailures() > 0) {
      printf("FAIL | %s (%d assertion(s) failed)\n", paths[i],
             testHelperFailures());
      test_failed = true;
    }

    if (!test_failed) {
      printf("PASS | %s\n", paths[i]);
      passed++;
    } else {
      failed++;
    }

    free(all_nodes);
    clearGlobalState(state, 10);
    free(content);
  }

  printf("\n> REPL boundary test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}
