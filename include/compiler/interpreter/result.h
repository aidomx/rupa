#pragma once
#if defined(RUPA_PACKAGE_H)

struct InterpreterResult {
  enum InterpreterFlow flow;
  struct RuntimeValue value;
};

#endif
