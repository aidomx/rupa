#include <rupa.h>
#include "test_internal.h"

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
    if (!state || !state->buffer) {
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
    builtinsInit(sharedEnv);
    testHelperInit(sharedEnv);
    testHelperReset();
    setSourceFilePath(paths[i]);
    analyzerReset(); /* registry struct per-file */

    bool test_failed = false;
    int line_num = 0;

    /* Mirror the interactive REPL (see processReplInput): lexer context and
     * accumulated input must survive across lines so multi-line constructs
     * (struct, object literal, function) hold via isWaiting and only execute
     * once complete. Resetting context/size per line broke every multi-line
     * REPL boundary test and made later lines execute stale tokens. */
    clearReplState(state->repl);
    clearInput(state->input);
    clearStateToken(state->tokens);
    clearStateContext(state->context);
    state->size = 0;

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

      state->buffer->value[0] = '\0';
      state->buffer->length = 0;

      size_t len = strlen(line);
      if ((int)len >= state->buffer->capacity ||
          (int)(state->input->length + len + 2) >= MAX_BUFFER_SIZE) {
        printf("  FAIL line %d (buffer overflow)\n", line_num);
        test_failed = true;
        break;
      }
      memcpy(state->buffer->value, line, len);
      state->buffer->value[len] = '\0';
      state->buffer->length = (int)len;

      printf("  %2d> %s\n", line_num, line);

      addToHistory(state);

      /* Build input content: append while multiline is in progress, replace
       * on fresh statement — same contract as processReplInput. */
      Input *input = state->input;
      if (input->length > 0) {
        memcpy(input->content + input->length, line, len);
        input->length += (int)len;
      } else {
        input->cursor = 0;
        memcpy(input->content, line, len);
        input->length = (int)len;
      }
      if (input->length > 0 && input->content[input->length - 1] != '\n')
        input->content[input->length++] = '\n';
      input->content[input->length] = '\0';

      lexer(state);

      Flags *flags = state->input->flags;
      Token *tokens = state->tokens;

      /* Multiline in progress — keep accumulated input, wait for more lines */
      if (flags && flags->isWaiting) {
        line = strtok_r(NULL, "\n", &saveptr);
        continue;
      }

      if (!tokens || tokens->length == 0) {
        line = strtok_r(NULL, "\n", &saveptr);
        continue;
      }

      {
        Request req = createRequest(tokens, 10);
        Node *node = processGenerate(&req);
        Error *error = createError(10);

        if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
          state->input->length = 0;
          clearStateToken(state->tokens);
          if (flags) resetFlags(flags);
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
          state->input->length = 0;
          clearStateToken(state->tokens);
          if (flags) resetFlags(flags);
          test_failed = true;
          line = strtok_r(NULL, "\n", &saveptr);
          continue;
        }

        InterpreterResult result = interpretNode(node, root, sharedEnv, error);

        /* Statement complete — flush accumulated input like the REPL does */
        state->input->length = 0;
        clearStateToken(state->tokens);
        if (flags) resetFlags(flags);

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
      printf("FAIL | %s (%d assertion(s) failed)\n", paths[i], testHelperFailures());
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
  printf("Cache  : memo %d/%d, canonical %d/%d (hit/miss)\n",
         syntaxMemoHits(), syntaxMemoMisses(), syntaxCanonicalHits(),
         syntaxCanonicalMisses());
}
