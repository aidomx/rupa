#include <rupa.h>

/* Runner file bebas (tidak terikat tests/): rupa ast|ir|irexec|exec|semantic|stress|repl <file.rp> */
static int runSingleFile(const char *cmd, const char *paths[], int length) {
  if (strcmp(cmd, "repl") == 0 && length < 1) {
    startRepl(true); /* rupa repl tanpa file = REPL interaktif */
    return 0;
  }
  if (length < 1) {
    fprintf(stderr, "rupa %s: file.rp diperlukan", cmd);
    if (strcmp(cmd, "exec") == 0 || strcmp(cmd, "semantic") == 0 ||
        strcmp(cmd, "stress") == 0)
      fprintf(stderr, " (batch: rupa test %s)", cmd);
    fprintf(stderr, "\n");
    return 1;
  }
  if (strcmp(cmd, "ast") == 0) {
    testAst(paths, length);
  } else if (strcmp(cmd, "ir") == 0) {
    testIR(paths, length);
  } else if (strcmp(cmd, "irexec") == 0) {
    testIRExec(paths, length);
  } else if (strcmp(cmd, "exec") == 0 || strcmp(cmd, "semantic") == 0 ||
             strcmp(cmd, "stress") == 0) {
    testExec(paths, length);
  } else if (strcmp(cmd, "repl") == 0) {
    testRepl(paths, length);
  } else {
    return 1;
  }
  return 0;
}

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

  /* args[0] adalah nama program, mulai parsing dari args[1]. */
  for (int i = 1; i < length; i++) {
    if (strcmp(args[i], "help") == 0 || strcmp(args[i], "--help") == 0) {
      /* help <topic> generik: topic = nama section di src/prompt/cmd.txt
       * (compiler, formatter, test, list, project, module, repl). */
      if (i + 1 < length && helpShowSection(args[i + 1])) {
        /* topic dikenal */
      } else if (i + 1 < length) {
        fprintf(stderr, "help: unknown topic '%s'\n", args[i + 1]);
        help(false);
        gcclean();
        return 1;
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

    else if (strcmp(args[i], "profile") == 0) {
      int result = profileRun(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "ast") == 0 || strcmp(args[i], "ir") == 0 ||
             strcmp(args[i], "irexec") == 0 || strcmp(args[i], "exec") == 0 ||
             strcmp(args[i], "semantic") == 0 || strcmp(args[i], "stress") == 0 ||
             strcmp(args[i], "repl") == 0) {
      /* Bentuk bebas single/multi file — tidak terikat tests/. */
      int result = runSingleFile(args[i], args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "test") == 0) {
      int result = testDispatch(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "-t") == 0 ||
             (args[i][0] == '-' && strspn(args[i] + 1, "tl") == strlen(args[i] + 1) &&
              strspn(args[i] + 1, "tl") > 0 && strchr(args[i], 't'))) {
      /* Cluster shortcut test: -t, -lt — diteruskan utuh ke testDispatch. */
      int result = testDispatch(args + i, length - i);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "-l") == 0) {
      /* -l = daftar modules; -l <category> = daftar file test kategori. */
      int result;
      if (i + 1 < length && args[i + 1][0] != '-') {
        const char *largs[] = {"-lt", args[i + 1]};
        result = testDispatch(largs, 2);
      } else {
        const char *largs[] = {"list"};
        result = stdlibManage(largs, 1);
      }
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "list") == 0) {
      /* list = modules; list tests [category] = daftar file test. */
      int result;
      if (i + 1 < length && strcmp(args[i + 1], "tests") == 0) {
        if (i + 2 < length) {
          const char *largs[] = {"-lt", args[i + 2]};
          result = testDispatch(largs, 2);
        } else {
          const char *largs[] = {"-lt"};
          result = testDispatch(largs, 1);
        }
      } else {
        result = stdlibManage(args + i, length - i);
      }
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "go") == 0) {
      int result = goCompile(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (strcmp(args[i], "spec") == 0) {
      int result = specCommand(args + i + 1, length - i - 1);
      handled = true;
      autorun = false;
      gcclean();
      return result;
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

    else if (strcmp(args[i], "--version") == 0) {
      version();
      handled = true;
      autorun = false;
      break;
    }

    else if (strcmp(args[i], "install") == 0 || strcmp(args[i], "add") == 0 ||
             strcmp(args[i], "update") == 0 || strcmp(args[i], "delete") == 0 ||
             strcmp(args[i], "remove") == 0) {
      int result = stdlibManage(args + i, length - i);
      handled = true;
      autorun = false;
      gcclean();
      return result;
    }

    else if (args[i][0] == '-') {
      /* Opsi tidak dikenal — jangan jatuh ke autorun (membaca file). */
      fprintf(stderr, "rupa: unknown option '%s' (lihat: rupa help)\n", args[i]);
      handled = true;
      autorun = false;
      gcclean();
      return 1;
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
