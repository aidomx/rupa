#pragma once
#if defined(RUPA_PACKAGE_H)
struct RuntimeFunction {
  Node *node;
  int name;
  int *params;
  int paramLength;
  int body;
  RuntimeEnv *closure;
};
#endif
