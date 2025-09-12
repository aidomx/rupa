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
  case MINUS:
    return BINARY_SUBTRACT;
  case PERCENT:
    return BINARY_MODULES;
  case PLUS:
    return BINARY_ADD;
  case SLASH:
    return BINARY_DIVIDE;
  case STAR:
    return BINARY_MULTIPLY;
  default:
    return BINARY_NONE;
  }
}

DataToken *getToken(Token *t, int pos) {
  return (pos < 0 || pos >= t->length) ? NULL : &t->data[pos];
}

/**
 * Mendapatkan precedence operator.
 * Angka lebih tinggi = precedence lebih kuat.
 */
int getPrecedence(DataToken *token) {
  if (!token)
    return -1;
  switch (token->type) {
  case PERCENT:
  case SLASH:
  case STAR:
    return 13;
  case MINUS:
  case PLUS:
    return 12;
  case ASSIGN:
    return 9;
  default:
    return -1;
  }
}
