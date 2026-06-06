#pragma once
/**
 * Definisi tipe yang digunakan untuk menentukan
 * tipe dari variabel saat proses membangun AST.
 *
 * @VariableType
 */
enum VariableType {
  VAR_BIGINT = 0,
  VAR_BOOLEAN = 1,
  VAR_DOUBLE = 2,
  VAR_FLOAT = 3,
  VAR_NUMBER = 4,
  VAR_STRING = 5,
  VAR_UNKNOWN = -1
};
