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

    else if (strcmp(args[i], "--test-ir") == 0) {
      testIR(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "--test-irexec") == 0) {
      testIRExec(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "fmt") == 0) {
      int result = 0;
      if (i + 1 >= length) {
        showFmtHelp();
        gcclean();
        return 1;
      }
      const char *sub = args[i + 1];
      if (strcmp(sub, "help") == 0 || strcmp(sub, "--help") == 0 || strcmp(sub, "-h") == 0) {
        showFmtHelp();
        gcclean();
        return 0;
      }
      if (strcmp(sub, "-") == 0) {
        result = formatStdin();
        gcclean();
        return result;
      }

      /* Flag fmt bersifat composable: --list, --path, --select dan --exclude
       * boleh dikombinasikan dalam urutan apa pun, mis.:
       *   rupa fmt --path syntax --select 41
       *   rupa fmt --select 41 --path syntax
       * Path kategori juga tetap diterima sebagai argumen posisi tunggal
       * agar kompatibel: rupa fmt --select 41 syntax */
      bool wantList = false;
      bool wantPath = false;
      const char *pathArg = NULL;
      const char *selectArg = NULL;
      const char *excludes[64];
      int excludeCount = 0;
      bool badUsage = false;

      for (int j = i + 1; j < length; j++) {
        const char *arg = args[j];
        if (strcmp(arg, "--list") == 0) {
          wantList = true;
        } else if (strcmp(arg, "--path") == 0) {
          if (j + 1 >= length || args[j + 1][0] == '-') {
            fprintf(stderr, "fmt: --path requires a path (e.g. syntax)\n");
            badUsage = true;
            break;
          }
          pathArg = args[++j];
          wantPath = true;
        } else if (strcmp(arg, "--select") == 0) {
          if (j + 1 >= length || args[j + 1][0] == '-') {
            fprintf(stderr, "fmt: --select requires indexes (e.g. 1,2,3)\n");
            badUsage = true;
            break;
          }
          selectArg = args[++j];
        } else if (strcmp(arg, "--exclude") == 0) {
          if (j + 1 >= length || args[j + 1][0] == '-') {
            fprintf(stderr, "fmt: --exclude requires a path\n");
            badUsage = true;
            break;
          }
          if (excludeCount < 64) excludes[excludeCount++] = args[++j];
        } else if (strncmp(arg, "--exclude=", 10) == 0) {
          if (excludeCount < 64) excludes[excludeCount++] = arg + 10;
        } else if (arg[0] == '-') {
          fprintf(stderr, "fmt: unknown option '%s'\n", arg);
          badUsage = true;
          break;
        } else if (!pathArg) {
          pathArg = arg; /* kategori/path posisi tunggal, mis. "syntax" */
        } else {
          fprintf(stderr, "fmt: unexpected argument '%s'\n", arg);
          badUsage = true;
          break;
        }
      }

      if (badUsage) {
        gcclean();
        return 1;
      }

      if (wantList) {
        result = formatList(pathArg ? pathArg : "tests", true);
      } else if (selectArg) {
        result = formatSelect(selectArg, pathArg, excludes, excludeCount);
      } else if (pathArg && wantPath) {
        result = formatList(pathArg, false);
      } else if (pathArg) {
        result = formatFile(pathArg);
      } else {
        showFmtHelp();
        result = 1;
      }
      gcclean();
      return result;
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

    else if (strcmp(args[i], "install") == 0 || strcmp(args[i], "add") == 0 ||
             strcmp(args[i], "update") == 0 || strcmp(args[i], "delete") == 0 ||
             strcmp(args[i], "remove") == 0 || strcmp(args[i], "list") == 0 ||
             strcmp(args[i], "-g") == 0) {
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
