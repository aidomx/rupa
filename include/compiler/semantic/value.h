#pragma once
#if defined(RUPA_PACKAGE_H)

struct Value {
  ValueType type;
  union {
    int number;
    double decimal;
    bool boolean;
    char *string;
  } as;
};

#endif
