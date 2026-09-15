#pragma once
#if defined(RUPA_PACKAGE_H)

struct StateInput {
  char *value;
  int line;
  int row;
  FlagType flag;
  struct StateInput *next;
};

#endif
