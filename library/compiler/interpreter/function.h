#pragma once
#if defined(RUPA_PACKAGE_H)
struct RuntimeFunction {
  Node *node;
  int name;
  int *params;
  int paramLength;
  int body;
  int returnType; /* node id return-type annotation — -1 jika tanpa */
  RuntimeEnv *closure;
};
#endif
