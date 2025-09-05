#include <rupa/package.h>

int main(int argc, char *argv[]) {
  if (argc > 1) {
    printf("%s\n", argv[0]);
  }

  startRepl();
  return 0;
}
