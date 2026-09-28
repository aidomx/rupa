#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * Syntax test: lex + parse + interpret, show output per file.
 * Same behavior as ./bin/rupa <file>, with PASS/FAIL summary.
 * ================================================================ */

void test(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(length, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
    return;
  }

  int passed = 0, failed = 0;

  for (int i = 0; i < length; i++) {
    if (state->repl) clearReplState(state->repl);

    clearInput(state->input);
    clearStateToken(state->tokens);
    clearStateContext(state->context);
    state->size = 0;

    /* Reset history so addToInput doesn't skip stale entries.
     * Without this, h->size stays from the previous test and
     * findNewStart() in addToInput() skips the newly added entry,
     * leaving input empty and causing cascading failures. */
    if (state->history) {
      state->history->size = 0;
      state->history->currentIndex = -1;
    }

    Buffer *buffer = state->buffer;

    if (!readfile(paths[i], buffer)) {
      printf("FAIL | Read file %s\n", paths[i]);
      failed++;
      continue;
    }

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

    /* Find program root */
    int root = -1;
    for (int j = 0; j < node->length; j++) {
      if (node->ast[j].type == NODE_PROGRAM) {
        root = j;
        break;
      }
    }
    if (root < 0) {
      printf("FAIL | %s (no program root)\n", paths[i]);
      failed++;
      continue;
    }

    /* Cache pipeline: simpan hasil generate — grup test lain dalam
     * proses yang sama (ir/irexec, file tests/syntax yang sama)
     * melompati lexer + generate + rewrite. */
    pipelineCachePut(paths[i], node, NULL);

    /* Execute — same as ./bin/rupa <file> */
    setSourceFilePath(paths[i]);
    RuntimeEnv *env = semCreateEnv(NULL);
    if (!env) {
      printf("FAIL | %s (env alloc failed)\n", paths[i]);
      failed++;
      continue;
    }
    stdlibInit(env);
    builtinsInit(env);

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

  State *state = createGlobalState(length, false);
  if (!state || !state->buffer) {
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

    if (!testLexParse(state, paths[i], &buffer, &tokens, &node)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    printf("\n--- %s ---\n", paths[i]);
    printf("Source:\n");
    testPrintSource(buffer->value);
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
