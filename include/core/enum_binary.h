#pragma once
/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing expression menjadi AST.
 *
 * @BinaryType
 */
enum BinaryType {
  BINARY_ADD = 0,
  BINARY_ASSIGN = 1,
  BINARY_MULTIPLY = 2,
  BINARY_SUBTRACT = 3,
  BINARY_SUBSCRIPT = 4,
  BINARY_DIVIDE = 5,
  BINARY_MODULES = 6,
  BINARY_NONE = -1,
};
