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

void version() {
  printf(formatVersion, RUPA_VERSION);
  printf("\n");
}
