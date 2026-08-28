#pragma once
/** Runtime value types used by interpreter. */
enum ValueType {
  VALUE_NULL,
  VALUE_NUMBER,
  VALUE_DECIMAL,
  VALUE_BOOLEAN,
  VALUE_STRING,
  VALUE_ARRAY,
  VALUE_FUNCTION,
  VALUE_OBJECT,
  VALUE_NATIVE_FUNCTION
};
