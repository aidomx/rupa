#include <rupa.h>

int loader(const char *args[], int length) {
  gcinit(100);
  stdlibLoaderInit(); /* Scan ~/.rupa/stdlib/ and ./stdlib/ */

  if (length <= 1) {
    startRepl(true);
    gcclean();
    return 0;
  }

  bool handled = false;
  bool autorun = true;
  int index = 0;

  for (int i = 0; i < length; i++) {
    if (strcmp(args[i], "help") == 0) {
      if (i + 1 < length && strcmp(args[i + 1], "module") == 0) {
        showModuleHelp();
      } else if (i + 1 < length && strcmp(args[i + 1], "test") == 0) {
        showTestHelp();
      } else {
        help(false);
      }
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--help") == 0) {
      if (i + 1 < length && strcmp(args[i + 1], "module") == 0) {
        showModuleHelp();
      } else if (i + 1 < length && strcmp(args[i + 1], "test") == 0) {
        showTestHelp();
      } else {
        help(false);
      }
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "-e") == 0) {
      if (i + 1 < length) {
        execute(args[i + 1]);
      } else {
        fprintf(stderr, "Error: -e requires a code argument.\n");
      }
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--test") == 0) {
      test(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--test-ast") == 0) {
      testAst(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--test-exec") == 0) {
      testExec(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--test-repl") == 0) {
      testRepl(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--version") == 0) {
      version();
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "add") == 0 || strcmp(args[i], "update") == 0 ||
             strcmp(args[i], "delete") == 0 || strcmp(args[i], "remove") == 0 ||
             strcmp(args[i], "list") == 0 || strcmp(args[i], "-g") == 0) {
      int result = stdlibManage(args + i, length - i);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    index = length - i;
  }

  if (autorun) {
    // rupa <file>
    int result = run(args, index);
    handled = true;
    if (result != 0) {
      gcclean();
      return result;
    }
  }

  if (!handled) {
    fprintf(stderr, "command is not found!\n");
    gcclean();
    return 1;
  }

  // cleanup
  gcclean();
  return 0;
}
