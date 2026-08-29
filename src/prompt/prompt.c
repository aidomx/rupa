#include <rupa.h>

// CLI commands (rupa --help di terminal)
static const char *cli_commands[] = {
    "rupa <file>     - Run Rupa script",
    "-e <code>       - Execute code string", "--help          - Show this help",
    "--version       - Show version information"};

// REPL commands (.help dalam REPL mode)
static const char *repl_commands[] = {
    ".clear for clear screen and history!",
    ".editor enter editor mode (is not ready used!)", ".exit for exit the repl",
    ".help for more information."};

static const char *formatVersion = "v%s";

void welcomeMessage() {
  printf("Welcome to Rupa ");
  printf(formatVersion, RUPA_VERSION);
  printf("\nPlease %s\n", repl_commands[3]); // .help information
}

void showCliHelp() {
  int length = sizeof(cli_commands) / sizeof(cli_commands[0]);
  printf("Usage: rupa [command] [options]\n\n");
  for (int i = 0; i < length; i++) {
    printf("  %s\n", cli_commands[i]);
  }
}

void showReplHelp() {
  int length = sizeof(repl_commands) / sizeof(repl_commands[0]);
  for (int i = 0; i < length; i++) {
    printf("%s\n", repl_commands[i]);
  }
}

/**
 * @param is_repl_mode true = repl help, false = cli help
 */
void help(bool is_repl_mode) {
  if (is_repl_mode) {
    showReplHelp();
  } else {
    showCliHelp();
  }
}

void run(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No such file for execute.\n");
    return;
  }

  State *state = createGlobalState(length, true);
  if (!state || !state->repl || !state->repl->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
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
    printf("FAIL | %s\n", index);
    return;
  }

  /* Set source file path for resolving relative imports */
  setSourceFilePath(index);

  processInput(state);

  Flags *flags = state->input->flags;
  if (!state->tokens || state->tokens->length == 0 ||
      !hasAstDeclarations(state->tokens) || (flags && flags->isWaiting)) {
    printf("FAIL | %s\n", index);
  }
}

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
    /* Each file is an independent syntax test. Keep the same State object,
     * but reset every stateful subsystem before loading the next source. */
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

    processInput(state);

    Flags *flags = state->input->flags;
    if (!state->tokens || state->tokens->length == 0 ||
        !hasAstDeclarations(state->tokens) || (flags && flags->isWaiting)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    printf("PASS | %s\n", paths[i]);
    passed++;
  }

  printf("\n> Test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
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

void version() {
  printf(formatVersion, RUPA_VERSION);
  printf("\n");
}
