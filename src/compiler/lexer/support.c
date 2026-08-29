#include <rupa.h>

char *getTokenId(const char *input, int start, int end) {
  return (!input || end == 0) ? NULL : substring(input, start, end);
}

char *getTokenValue(const char *input, int start, int end) {
  return (!input || end == 0) ? NULL : substring(input, start, end);
}


