#include <rupa.h>

static const char *cli_commands[] = {
    "rupa <file>         - Run a Rupa script",
    "-e <code>           - Execute code",
    "--help|help         - Show this help",
    "--help|help <topic> - Show topic module, test, etc",
    "--version           - Show version"};

static const char *module_commands[] = {
    "add [-g] <name> <path> [version] - Install a module",
    "remove [-g] <name>              - Remove a module",
    "list [-g]                       - List installed modules",
    "update [-g] <name> <path>       - Update a module",
    "",
    "-g installs to the global archive: ~/.rupa/rupa_modules.tar.gz",
    "without -g, modules are stored locally: ./modules/rupa_modules.tar.gz",
    "",
    "Import local:  import name from name",
    "Import global: import name from rupa.name"};

static const char *test_commands[] = {
    "--test <file>       - Run syntax and execution tests",
    "--test-ast <file>   - Parse and show the AST",
    "--test-exec <file>  - Run execution tests with assertions",
    "--test-repl <file>  - Test multi-line REPL execution",
    "",
    "Examples:",
    "  rupa --test tests/syntax/module.rp",
    "  rupa --test-exec tests/execution/*.rp"};

static const char *repl_commands[] = {
    ".clear for clear screen and history!",
    ".editor enter editor mode (is not ready used!)", ".exit for exit the repl",
    ".help for more information."};

static const char *formatVersion = "Rupa v%s";

void welcomeMessage() {
  printf("Welcome to Rupa ");
  printf(formatVersion, RUPA_VERSION);
  printf("\nPlease %s\n", repl_commands[3]);
}

void showCliHelp() {
  int length = sizeof(cli_commands) / sizeof(cli_commands[0]);
  printf("Usage: rupa --help | help <topic>\n\n");
  for (int i = 0; i < length; i++) {
    printf("  %s\n", cli_commands[i]);
  }
}

void showModuleHelp() {
  int length = sizeof(module_commands) / sizeof(module_commands[0]);
  printf("Usage: rupa <command> [options]\n\nModule commands:\n");
  for (int i = 0; i < length; i++) {
    printf("  %s\n", module_commands[i]);
  }
}

void showTestHelp() {
  int length = sizeof(test_commands) / sizeof(test_commands[0]);
  printf("Usage: rupa --test[-ast|-exec|-repl] <file>...\n\nTest commands:\n");
  for (int i = 0; i < length; i++) {
    printf("  %s\n", test_commands[i]);
  }
}

void showReplHelp() {
  int length = sizeof(repl_commands) / sizeof(repl_commands[0]);
  for (int i = 0; i < length; i++) {
    printf("%s\n", repl_commands[i]);
  }
}

void help(bool is_repl_mode) {
  if (is_repl_mode) {
    showReplHelp();
  } else {
    showCliHelp();
  }
}

void version() {
  printf(formatVersion, RUPA_VERSION);
  printf("A general-purpose programming language\n");
  printf("\n");
}
