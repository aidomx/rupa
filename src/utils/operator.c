#include <rupa/package.h>

/**
 * Mengubah token operator menjadi enum BinaryType internal.
 */
BinaryType getBinaryType(DataToken *token) {
  if (!token)
    return BINARY_NONE;
  switch (token->type) {
  case ASSIGN:
    return BINARY_ASSIGN;
  case ASTERISK:
    return BINARY_MULTIPLY;
  case MINUS:
    return BINARY_SUBTRACT;
  case PLUS:
    return BINARY_ADD;
  case SLASH:
    return BINARY_DIVIDE;
  default:
    return BINARY_NONE;
  }
}

/**
 * Mendapatkan precedence operator.
 * Angka lebih tinggi = precedence lebih kuat.
 */
int getPrecedence(DataToken *token) {
  if (!token)
    return -1;
  switch (token->type) {
  case ASTERISK:
  case SLASH:
  case STAR:
    return 3;
  case MINUS:
  case PLUS:
    return 2;
  case ASSIGN:
    return 1;
  default:
    return -1;
  }
}
