#pragma once

#if defined(RUPA_PACKAGE_H)

struct RuntimeBinding {
  char *name;
  char *type;
  RuntimeValue value;
  struct RuntimeBinding *next;
};

struct RuntimeEnv {
  struct RuntimeEnv *parent;
  struct RuntimeBinding *bindings;
};

#endif
