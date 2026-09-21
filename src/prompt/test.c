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
static bool lexParse(State *state, const char *path, Buffer **outBuf, Token **outTokens,
                     Node **outNode) {
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  /* Reset history so addToInput doesn't skip stale entries. */
  if (state->history) {
    state->history->size = 0;
    state->history->currentIndex = -1;
  }

  Buffer *buffer = state->buffer;
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
  if (!tokens || tokens->length == 0 || (state->input->flags && state->input->flags->isWaiting)) {
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
    Buffer *buffer;
    Token *tokens;
    Node *node;

    if (!lexParse(state, paths[i], &buffer, &tokens, &node)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    IRModule *ir = createIR();
    if (!ir || !rewrite(node, -1, ir)) {
      printf("FAIL | %s (rewrite failed)\n", paths[i]);
      failed++;
      continue;
    }

    printf("\n--- %s ---\n", paths[i]);
    debugIRModule(ir);
    printf("PASS\n");
    passed++;
    irModuleFree(ir);
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
    Node *node = processGenerate(&request);
    if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
      printf("FAIL | %s (parse failed)\n", paths[i]);
      failed++;
      continue;
    }

    setSourceFilePath(paths[i]);

    IRModule *ir = createIR();
    if (!ir || !rewrite(node, -1, ir)) {
      printf("FAIL | %s (rewrite failed)\n", paths[i]);
      failed++;
      continue;
    }

    testHelperReset();
    analyzerReset(); /* registry struct per-file */

    printf("\n--- %s ---\n", paths[i]);
    Error *execError = createError(10);
    int execStatus = executeIRErrorWithEnv(ir, node, execError, executeIRRegisterHelpers);
    irModuleFree(ir);

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
    printSource(buffer->value);
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

/* ================================================================
 * Formatter test: show original source vs formatted output per file.
 * ================================================================ */

void testFmt(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  int modified = 0;
  int unchanged = 0;
  int failed = 0;

  printf("> Formatter tests\n");

  for (int i = 0; i < length; i++) {
    char *source = fmtReadSource(paths[i]);

    /* fmtRunFile owns its GC cleanup; reinitialize before each file
     * when invoked repeatedly in batch mode (see fmtRunPaths). */
    gcinit(100);

    char *formatted = NULL;
    size_t formattedLen = 0;
    FILE *fp = open_memstream(&formatted, &formattedLen);
    int r = fp ? fmtRunFile(paths[i], fp) : 1;
    if (fp) fclose(fp);

    printf("\n--- %s ---\n", paths[i]);
    if (r != 0 || !formatted) {
      printf("FAILED | %s\n", paths[i]);
      failed++;
      free(source);
      free(formatted);
      continue;
    }

    printf("Source:\n");
    if (source && *source) {
      fputs(source, stdout);
      if (source[strlen(source) - 1] != '\n') printf("\n");
    } else {
      printf("(empty)\n");
    }

    bool changed =
        !source || strlen(source) != formattedLen || memcmp(source, formatted, formattedLen) != 0;

    printf("%s:\n", changed ? "Formatted" : "Formatted (unchanged)");
    fwrite(formatted, 1, formattedLen, stdout);
    if (formattedLen == 0 || formatted[formattedLen - 1] != '\n') printf("\n");
    printf("STATUS | %s\n", changed ? "MODIFIED" : "UNCHANGED");

    if (changed)
      modified++;
    else
      unchanged++;
    free(source);
    free(formatted);
  }

  printf("\n> Formatter test summary\n");
  printf("Modified : %d\n", modified);
  printf("Unchanged : %d\n", unchanged);
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
}

/* ================================================================
 * Test dispatcher — satu pintu untuk rupa test [...]
 *
 * Perilaku:
 *   rupa test --list                → tampilkan semua tests
 *   rupa test --list ast            → tampilkan tests/ast
 *   rupa test                       → jalankan tests/syntax
 *   rupa test ast                   → jalankan tests/ast
 *   rupa test --path ast            → sama dengan "rupa test ast"
 *   rupa test --select 1,3          → filter dari tests/syntax
 *   rupa test ast --select 1        → filter dari tests/ast
 *   rupa test --path ast --select 1 → sama dengan di atas (urutan bebas)
 *   rupa test exec                  → tests/execution (--test-exec)
 *   rupa test fmt                   → source vs hasil formatter
 *
 * Kategori yang dikenali: syntax, ast, ir, irexec, exec, semantics,
 * repl, fmt (posisi atau lewat --path, boleh berulang — yang terakhir menang).
 * ================================================================ */

static const struct {
  const char *name;
  const char *dir;  /* subfolder di bawah tests/ */
  const char *flag; /* flag binary yang dipakai */
} testCategories[] = {
    {"syntax", "syntax", "--test"},       {"ast", "ast", "--test-ast"},
    {"ir", "syntax", "--test-ir"},        {"irexec", "syntax", "--test-irexec"},
    {"exec", "execution", "--test-exec"}, {"semantics", "semantics", "--test-exec"},
    {"repl", "execution", "--test-repl"}, {"fmt", "formatter", "fmt"},
};

static const char *testFindCategoryDir(const char *name) {
  if (!name) return NULL;
  for (size_t k = 0; k < sizeof(testCategories) / sizeof(testCategories[0]); k++) {
    if (strcmp(testCategories[k].name, name) == 0) return testCategories[k].dir;
  }
  return NULL;
}

static const char *testFindCategoryFlag(const char *name) {
  if (!name) return NULL;
  for (size_t k = 0; k < sizeof(testCategories) / sizeof(testCategories[0]); k++) {
    if (strcmp(testCategories[k].name, name) == 0) return testCategories[k].flag;
  }
  return NULL;
}

static void testUsage(void) {
  showTestHelp();
}

/* Bangun FmtPathList dari folder test + filter kata kunci opsional.
 * Hanya file .rp di dalam folder yang dikumpulkan, lalu diurutkan —
 * penomoran --select mengikuti urutan ini. */
static int testCollectPaths(const char *dir, const char *keyword, FmtPathList *list) {
  char root[4096];
  snprintf(root, sizeof(root), "tests/%s", dir);

  /* Validasi kategori: folder harus ada agar salah ketik langsung terlihat. */
  struct stat st;
  if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "test: test directory not found: %s\n", root);
    return 1;
  }

  if (fmtCollectRp(root, list) != 0) return 1;
  qsort(list->items, (size_t)list->count, sizeof(*list->items), fmtPathCmp);

  /* Filter kata kunci (substring match, mis. "repl" di folder execution). */
  if (keyword && *keyword) {
    int kept = 0;
    for (int i = 0; i < list->count; i++) {
      if (strstr(list->items[i], keyword)) list->items[kept++] = list->items[i];
    }
    for (int i = kept; i < list->count; i++)
      free(list->items[i]);
    list->count = kept;
  }
  return 0;
}

/* Parse "1,3,7" → daftar index 1-based. Pembagi milik pemanggil. */
static int *testParseSelection(const char *value, int *outCount) {
  *outCount = 0;
  if (!value || !*value) return NULL;

  int capacity = 8;
  int *items = malloc((size_t)capacity * sizeof(*items));
  if (!items) return NULL;

  char *copy = strdup(value);
  if (!copy) {
    free(items);
    return NULL;
  }

  char *save = NULL;
  int count = 0;
  for (char *tok = strtok_r(copy, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    char *end = NULL;
    long n = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || n <= 0 || n > INT_MAX) {
      fprintf(stderr, "test: invalid selection '%s'\n", tok);
      free(items);
      free(copy);
      return NULL;
    }
    if (count >= capacity) {
      capacity *= 2;
      int *tmp = realloc(items, (size_t)capacity * sizeof(*items));
      if (!tmp) {
        free(items);
        free(copy);
        return NULL;
      }
      items = tmp;
    }
    items[count++] = (int)n;
  }
  free(copy);
  *outCount = count;
  return items;
}

/* Pilih file dari list berdasarkan index 1-based --select. */
static char **testSelectFiles(const FmtPathList *list, const int *selected, int selectedCount) {
  char **paths = malloc((size_t)selectedCount * sizeof(*paths));
  if (!paths) return NULL;
  for (int i = 0; i < selectedCount; i++) {
    if (selected[i] > list->count) {
      fprintf(stderr, "test: selection %d is out of range (1-%d)\n", selected[i], list->count);
      free(paths);
      return NULL;
    }
    paths[i] = list->items[selected[i] - 1];
  }
  return paths;
}

/* Jalankan satu grup: dispatch ke runner sesuai flag binary. */
static int testRunGroup(const char *flag, char **paths, int count) {
  if (strcmp(flag, "fmt") == 0) {
    testFmt((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test") == 0) {
    test((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test-ast") == 0) {
    testAst((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test-ir") == 0) {
    testIR((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test-irexec") == 0) {
    testIRExec((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test-exec") == 0) {
    testExec((const char **)paths, count);
    return 0;
  }
  if (strcmp(flag, "--test-repl") == 0) {
    testRepl((const char **)paths, count);
    return 0;
  }
  return 1;
}

/*
 * Parser argumen test (dipakai testDispatch).
 *
 * Aturan penempatan token posisi (agar bentuk cluster tetap masuk akal):
 *   - token numerik (boleh koma)     → nilai select (setara --select/-s)
 *   - token non-numerik pertama      → kategori (setara --path/-p)
 *   - token non-numerik kedua        → kategori grup kedua
 *
 * Contoh yang semuanya valid:
 *   test syntax 1        -t syntax 1        -ts syntax 1      -tps syntax 1
 *   test ast --select 1  -t ast --select 1  -t -p syntax -s 1  -l ast
 */
typedef struct {
  bool wantList;
  const char *selectArg;     /* nilai --select / -s (atau NULL) */
  const char *category;      /* kategori (posisi atau --path) */
  const char *extraCategory; /* kategori posisi kedua (atau NULL) */
} TestArgs;

/* "1", "1,2,3" dianggap token selection; kategori tidak mungkin numerik. */
static bool testIsSelectionToken(const char *arg) {
  if (!arg || !*arg) return false;
  for (const char *c = arg; *c; c++) {
    if (*c != ',' && (*c < '0' || *c > '9')) return false;
  }
  return true;
}

static bool testArgsParse(const char *args[], int length, TestArgs *a) {
  memset(a, 0, sizeof(*a));
  bool wantPath = false;
  bool wantSelect = false;

  for (int i = 0; i < length; i++) {
    const char *arg = args[i];

    if (strcmp(arg, "--list") == 0) {
      a->wantList = true;
      continue;
    }
    if (strcmp(arg, "--path") == 0) {
      wantPath = true;
      continue;
    }
    if (strcmp(arg, "--select") == 0) {
      wantSelect = true;
      continue;
    }

    if (arg[0] == '-' && arg[1] != '\0') {
      /* Cluster pendek: -t, -l, -lp, -ts, -tps, ... (hanya huruf t/l/p/s). */
      bool ok = true;
      for (const char *c = arg + 1; *c; c++) {
        switch (*c) {
        case 't':
          break; /* penanda mode test, tanpa efek */
        case 'l':
          a->wantList = true;
          break;
        case 'p':
          wantPath = true;
          break;
        case 's':
          wantSelect = true;
          break;
        default:
          ok = false;
          break;
        }
      }
      if (!ok) {
        fprintf(stderr, "test: unknown option '%s'\n", arg);
        testUsage();
        return false;
      }
      continue;
    }

    if (arg[0] == '-') {
      fprintf(stderr, "test: unknown option '%s'\n", arg);
      testUsage();
      return false;
    }

    /* Token posisi: numerik → select, non-numerik → kategori. */
    if (testIsSelectionToken(arg)) {
      if (a->selectArg) {
        fprintf(stderr, "test: unexpected argument '%s'\n", arg);
        testUsage();
        return false;
      }
      a->selectArg = arg;
    } else if (!a->category) {
      a->category = arg;
    } else if (!a->extraCategory && strcmp(arg, a->category) != 0) {
      a->extraCategory = arg;
    } else {
      fprintf(stderr, "test: unexpected argument '%s'\n", arg);
      testUsage();
      return false;
    }
  }

  if (wantPath && !a->category && !a->extraCategory) {
    fprintf(stderr, "test: --path requires a category (e.g. syntax, ast)\n");
    return false;
  }
  if (wantSelect && !a->selectArg) {
    fprintf(stderr, "test: --select requires indexes (e.g. 1,2,3)\n");
    return false;
  }
  return true;
}

int testDispatch(const char *args[], int length) {
  TestArgs a;
  if (!testArgsParse(args, length, &a)) return 1;

  const char *category = a.category;

  const char *keyword = NULL;
  if (category && !testFindCategoryFlag(category)) {
    /* Bukan kategori yang dikenal → perlakukan sebagai kata kunci filter. */
    keyword = category;
    category = NULL;
  }
  const char *extraCategory = a.extraCategory;
  if (extraCategory && !testFindCategoryFlag(extraCategory) && !keyword) {
    keyword = extraCategory;
    extraCategory = NULL;
  }

  const char *dir = category ? testFindCategoryDir(category) : "syntax";

  /* --- Mode list --- */
  if (a.wantList) {
    if (category || keyword) {
      /* -l <kategori>: daftar folder kategori (keyword boleh menfilter). */
      FmtPathList list = {0};
      if (testCollectPaths(dir, keyword, &list) != 0) return 1;
      fmtPrintList(&list, NULL, 0);
      fmtPathListFree(&list);
    } else if (extraCategory) {
      fprintf(stderr, "test: unexpected argument '%s'\n", extraCategory);
      testUsage();
      return 1;
    } else {
      /* Tanpa kategori: daftar semua file .rp di tests/. */
      FmtPathList list = {0};
      if (fmtCollectRp("tests", &list) != 0) return 1;
      qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
      fmtPrintList(&list, NULL, 0);
      fmtPathListFree(&list);
    }
    return 0;
  }

  /* --- Mode jalankan --- */
  const char *flag = testFindCategoryFlag(category ? category : "syntax");

  if (extraCategory) {
    const char *flag2 = testFindCategoryFlag(extraCategory);
    if (!flag2) {
      fprintf(stderr, "test: unknown category '%s'\n", extraCategory);
      testUsage();
      return 1;
    }
    if (a.selectArg) {
      fprintf(stderr, "test: --select cannot be combined with two categories\n");
      return 1;
    }
    const char *dir2 = testFindCategoryDir(extraCategory);

    FmtPathList l1 = {0};
    FmtPathList l2 = {0};
    if (testCollectPaths(dir, keyword, &l1) != 0) return 1;
    if (testCollectPaths(dir2, NULL, &l2) != 0) {
      fmtPathListFree(&l1);
      return 1;
    }
    int r = testRunGroup(flag, l1.items, l1.count);
    if (r == 0) r = testRunGroup(flag2, l2.items, l2.count);
    fmtPathListFree(&l1);
    fmtPathListFree(&l2);
    return r;
  }

  FmtPathList list = {0};
  if (testCollectPaths(dir, keyword, &list) != 0) return 1;

  int result = 0;
  if (a.selectArg) {
    int selectedCount = 0;
    int *selected = testParseSelection(a.selectArg, &selectedCount);
    if (!selected || selectedCount == 0) {
      fprintf(stderr, "test: --select requires indexes (e.g. 1,2,3)\n");
      free(selected);
      fmtPathListFree(&list);
      return 1;
    }
    char **paths = testSelectFiles(&list, selected, selectedCount);
    if (!paths) {
      free(selected);
      fmtPathListFree(&list);
      return 1;
    }
    result = testRunGroup(flag, paths, selectedCount);
    free(paths);
    free(selected);
  } else {
    if (list.count == 0) {
      printf("No .rp files found in tests/%s.\n", dir);
      fmtPathListFree(&list);
      return 1;
    }
    result = testRunGroup(flag, list.items, list.count);
  }

  fmtPathListFree(&list);
  return result;
}
