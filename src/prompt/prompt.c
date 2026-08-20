#include <rupa.h>

// CLI commands (rupa --help di terminal)
static const char *cli_commands[] = {
    "run <file>      - Run Rupa script",
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
    Buffer *buffer = state->repl->buffer;
    resetEditorState(state->repl);
    buffer->length = 0;
    buffer->value[0] = '\0';

    if (!readfile(paths[i], buffer)) {
      printf("FAIL | %s\n", paths[i]);
      failed++;
      continue;
    }

    processInput(state);
    printf("PASS | %s\n", paths[i]);
    passed++;
  }

  printf("\n> Test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}

void version() {
  printf(formatVersion, RUPA_VERSION);
  printf("\n");
}
