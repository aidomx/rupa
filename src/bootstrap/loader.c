#include <rupa.h>

int loader(const char *args[], int length) {
  gcinit(100);

  if (length <= 1) {
    startRepl(true);
    gcclean();
    return 0;
  }

  bool handled = false;

  for (int i = 0; i < length; i++) {
    if (strcmp(args[i], "--help") == 0) {
      help(false);
      handled = true;
      break;
    }

    else if (strcmp(args[i], "--test") == 0) {
      test(args + i + 1, length - i - 1);
      handled = true;
      break;
    }

    else if (strcmp(args[i], "--version") == 0) {
      version();
      handled = true;
      break;
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
