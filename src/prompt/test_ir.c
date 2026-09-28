#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * IR test: lex + parse + rewrite (AST -> IR), show IR structure.
 * ================================================================ */

void testIR(const char *paths[], int length) {
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

  for (int i = 0; i < length; i++) {
    Node *node = NULL;
    IRModule *ir = NULL;

    /* Cache pipeline: file yang sudah di-pipeline grup sebelumnya
     * (mis. `test syntax ir`) dipakai ulang tanpa lex + parse ulang. */
    if (!pipelineCacheGet(paths[i], &node, &ir)) {
      Buffer *buffer;
      Token *tokens;
      if (!testLexParse(state, paths[i], &buffer, &tokens, &node)) {
        printf("FAIL | %s\n", paths[i]);
        failed++;
        continue;
      }
      pipelineCachePut(paths[i], node, NULL);
    }

    /* Entry AST-only (ir NULL): bangun IR dari AST cache lalu
     * upgrade entry — file yang sama di grup berikutnya full hit. */
    if (!ir) {
      ir = createIR();
      if (!ir || !rewrite(node, -1, ir)) {
        printf("FAIL | %s (rewrite failed)\n", paths[i]);
        failed++;
        continue;
      }
      pipelineCachePut(paths[i], node, ir);
    }

    printf("\n--- %s ---\n", paths[i]);
    debugIRModule(ir);
    printf("PASS\n");
    passed++;
    /* ir dimiliki cache pipeline — tidak di-free di sini. */
  }

  printf("\n> IR test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}

/* ================================================================
 * IR execution test: rewrite (AST -> IR) lalu jalankan IR machine.
 * ================================================================ */

void testIRExec(const char *paths[], int length) {
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

  printf("> IR execution tests\n");

  for (int i = 0; i < length; i++) {
    Node *cachedNode = NULL;
    IRModule *cachedIr = NULL;

    /* Cache pipeline (lihat testIR): hit = tanpa lex + parse + rewrite. */
    if (!pipelineCacheGet(paths[i], &cachedNode, &cachedIr)) {
      if (state->repl) clearReplState(state->repl);
      clearInput(state->input);
      clearStateToken(state->tokens);
      clearStateContext(state->context);
      state->size = 0;
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
      cachedNode = processGenerate(&request);
      if (!cachedNode || cachedNode->length <= 0 || !hasAstDeclarations(tokens)) {
        printf("FAIL | %s (parse failed)\n", paths[i]);
        failed++;
        continue;
      }

      pipelineCachePut(paths[i], cachedNode, NULL);
    }

    /* IR: dari cache bila grup sebelumnya sudah membangunnya (mis.
     * `test ir irexec` — file tests/syntax yang sama). */
    if (!cachedIr) {
      cachedIr = createIR();
      if (!cachedIr || !rewrite(cachedNode, -1, cachedIr)) {
        printf("FAIL | %s (rewrite failed)\n", paths[i]);
        failed++;
        continue;
      }
      pipelineCachePut(paths[i], cachedNode, cachedIr);
    }

    setSourceFilePath(paths[i]);

    testHelperReset();
    analyzerReset(); /* registry struct per-file */

    printf("\n--- %s ---\n", paths[i]);
    Error *execError = createError(10);
    int execStatus =
        executeIRErrorWithEnv(cachedIr, cachedNode, execError, executeIRRegisterHelpers);
    /* cachedIr dimiliki cache pipeline — tidak di-free di sini. */

    if (execStatus != 0) {
      printf("FAIL | %s\n", paths[i]);
      if (execError && execError->size > 0) printErrors(execError);
      failed++;
      continue;
    }

    printf("PASS | %s\n", paths[i]);
    passed++;
  }

  printf("\n> IR execution test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}
