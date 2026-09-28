#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * Execution test: parse + interpret, show source + results.
 * ================================================================ */

void testExec(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(length, false);
  if (!state || !state->buffer) {
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

    if (state->repl) clearReplState(state->repl);
    clearInput(state->input);
    clearStateToken(state->tokens);
    clearStateContext(state->context);
    state->size = 0;

    /* Reset history between tests to prevent cascading failures. */
    if (state->history) {
      state->history->size = 0;
      state->history->currentIndex = -1;
    }

    Buffer *buffer = state->buffer;

    if (!readfile(paths[i], buffer)) {
      printf("FAIL | %s (file not found)\n", paths[i]);
      failed++;
      continue;
    }

    /* Show program source */
    printf("\n--- %s ---\n", paths[i]);
    printf("Source:\n");
    testPrintSource(buffer->value);
    printf("\n");

    addToHistory(state);
    addToInput(state);
    lexer(state);

    Flags *flags = state->input->flags;
    Token *tokens = state->tokens;

    if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
      printf("FAIL | %s (lex failed)", paths[i]);
      if (!tokens)
        printf(" — tokens is NULL");
      else if (tokens->length == 0)
        printf(" — no tokens produced");
      else if (flags && flags->isWaiting)
        printf(" — incomplete input (isWaiting)");
      printf("\n");
      failed++;
      continue;
    }

    Request request = createRequest(tokens, 10);
    Node *node = processGenerate(&request);
    Error *error = createError(10);

    if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
      printf("FAIL | %s (parse failed)", paths[i]);
      if (!node)
        printf(" — AST node is NULL");
      else if (node->length <= 0)
        printf(" — AST is empty");
      else
        printf(" — no declarations found");
      printf("\n");
      failed++;
      continue;
    }

    testHelperReset();
    setSourceFilePath(paths[i]);
    analyzerReset(); /* registry struct per-file */
    RuntimeEnv *env = semCreateEnv(NULL);
    if (!env) {
      printf("FAIL | %s (env alloc failed)\n", paths[i]);
      failed++;
      continue;
    }
    stdlibInit(env);
    builtinsInit(env);
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
      if (!exec_ok) printf("       execution error\n");
      if (!assert_ok) printf("       %d assertion(s) failed\n", testHelperFailures());
      if (!no_errors) printErrors(error);
      failed++;
    }
  }

  printf("\n> Execution test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}
