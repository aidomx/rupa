#pragma once

enum InterpreterFlow {
  FLOW_NORMAL = 0,
  FLOW_RETURN,
  FLOW_BREAK,
  FLOW_CONTINUE,
  FLOW_ERROR
};
