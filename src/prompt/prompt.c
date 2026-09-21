#include <rupa.h>

static const char *cli_commands[] = {
    "rupa <file>         - Run a Rupa script", "-e <code>           - Execute code",
    "--help|help         - Show this help", "--help|help <topic> - Show topic module, test, etc",
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
    "--test-ir <file>    - Parse, rewrite AST to IR, show the IR",
    "--test-irexec <file> - Rewrite AST to IR, execute via IR machine",
    "--test-exec <file>  - Run execution tests with assertions",
    "--test-fmt <file>   - Show original vs formatted source",
    "--test-repl <file>  - Test multi-line REPL execution",
    "",
    "Batch runner:",
    "  rupa test [--list] [--path <cat>] [--select <n,n>] [category]",
    "  Categories: syntax, ast, ir, irexec, exec, semantics, repl, fmt",
    "  Default category is syntax (tests/syntax/*.rp).",
    "",
    "Options:",
    "  -l, --list           List test files (all tests/ or one category)",
    "  -p, --path <cat>     Pick category; same as positional category",
    "  -s, --select <n,n>   Run selected files by listed number",
    "  -t                   Test shortcut, e.g. rupa -t -p syntax -s 1",
    "",
    "Examples:",
    "  rupa test                      Run tests/syntax/*.rp",
    "  rupa test --list               List all tests/**/*.rp",
    "  rupa test --list ast           List tests/ast/**/*.rp",
    "  rupa test ast                  Run tests/ast/**/*.rp",
    "  rupa test --path ast --select 1  Combine flags in any order",
    "  rupa test exec                 Run tests/execution/*.rp",
    "  rupa test fmt                  Show original vs formatted output",
    "",
    "Shortcuts:",
    "  rupa -t -p syntax -s 1",
    "  rupa -tps syntax 1",
    "  rupa -ts ast 1",
    "  rupa -l",
    "  rupa -lp ast",
    "",
    "Examples (single file):",
    "  rupa --test tests/syntax/module.rp",
    "  rupa --test-ir tests/syntax/module.rp",
    "  rupa --test-irexec tests/syntax/module.rp",
    "  rupa --test-exec tests/execution/*.rp"};

static const char *repl_commands[] = {".clear for clear screen and history!",
                                      ".editor enter editor mode (is not ready used!)",
                                      ".exit for exit the repl", ".help for more information."};

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

void showFmtHelp() {
  printf("Usage: rupa fmt <file> | - | <command> [options]\n\n");
  printf("Commands:\n");
  printf("  rupa fmt <file.rp>                  Format one file\n");
  printf("  rupa fmt -                          Format source from stdin\n");
  printf("  rupa fmt --list \"syntax\"           List tests/syntax files\n");
  printf("  rupa fmt --list                    List all tests/*.rp files\n");
  printf("  rupa fmt --select \"1,2,3\" [path]  Format selected test files\n");
  printf("  rupa fmt --path \"syntax\"           Format files with status + source\n");
  printf("  rupa fmt --path syntax --select 41  Combine flags in any order\n");
  printf("  rupa fmt help                      Show this help\n\n");
  printf("Options:\n");
  printf("  --exclude <path>                   Exclude a path; may be repeated\n");
  printf("  --exclude=<path>                   Same as above\n\n");
  printf("Selection indexes follow the sorted order shown by --list.\n");
  printf("An optional path (e.g. \"syntax\") scopes --list, --select and --path.\n");
}

void showTestHelp() {
  int length = sizeof(test_commands) / sizeof(test_commands[0]);
  printf("Usage: rupa test [options] | rupa --test[-ast|-ir|-irexec|-exec|-fmt|-repl] <file>...\n\nTest commands:\n");
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
  printf("\nA general-purpose programming language\n");
}
