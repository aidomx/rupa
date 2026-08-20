#include <rupa.h>

int main(int argc, const char *argv[]) {
  // forward to bootstrap loader
  return loader(argv, argc);
}
