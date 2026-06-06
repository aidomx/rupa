#pragma once
/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing menjadi AST.
 *
 * @NodeType
 */
enum NodeType {
  NODE_UNKNOWN = -1,
  NODE_ARRAY = 16,
  NODE_ASSIGN = 0,
  NODE_BOOLEAN = 1,
  NODE_BINARY = 2,
  NODE_ENDPROGRAM = 3,
  NODE_FLOAT = 4,
  NODE_FUNCTION = 5,
  NODE_IDENTIFIER = 6,
  NODE_LITERAL_ID = 7,
  NODE_NUMBER = 8,
  NODE_NULLABLE = 9,
  NODE_PROGRAM = 10,
  NODE_PARAM = 11,
  NODE_RETURN = 12,
  NODE_STRING = 13,
  NODE_SUBSCRIPT = 14,
  NODE_STRUCT = 17,
  NODE_VARIABLE = 15
};
