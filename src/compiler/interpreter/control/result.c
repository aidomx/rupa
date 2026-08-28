#include <rupa.h>

InterpreterResult resultNormal(RuntimeValue value) {
  return (InterpreterResult){.flow = FLOW_NORMAL, .value = value};
}

InterpreterResult resultFlow(InterpreterFlow flow, RuntimeValue value) {
  return (InterpreterResult){.flow = flow, .value = value};
}
