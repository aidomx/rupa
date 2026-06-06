#include <rupa.h>

int main(int argc, const char *argv[]) {
  if (argc > 3) {
    perror("3 arguments maximal!\n");
    return 0;
  }

  // forward to bootstrap loader
  return loader(argv, argc);
}
