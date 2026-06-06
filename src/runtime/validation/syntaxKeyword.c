#include <rupa.h>

/**
 * @brief Validasi apakah keyword memiliki blok pembuka yang valid.
 * Bisa berupa `{` atau `:`
 */
bool isValidBlock(const char *buffer, int *position) {
  if (!buffer)
    return false;
  skipWhitespace(buffer, position);
  while (buffer[*position]) {
    if (islblock(buffer[*position]) || iscolon(buffer[*position])) {
      return true;
    }
    (*position)++;
  }
  return false;
}

/**
 * @brief Validasi format setelah keyword import|export|extends.
 * Memastikan ada nama modul/path setelah keyword.
 */
bool isValidModule(const char *buffer, int *position) {
  if (!buffer)
    return false;

  skipWhitespace(buffer, position);
  while (buffer[*position]) {
    if (buffer[*position] && !isspace(buffer[*position]) &&
        buffer[*position] != '\0') {
      return true;
    }

    (*position)++;
  }
  return false;
}

/**
 * @brief Validasi format setelah keyword `print`.
 * Mengecek apakah ada tanda kurung buka '(' setelah print.
 */
bool isValidPrint(const char *buffer, int *position) {
  if (!buffer)
    return false;

  skipWhitespace(buffer, position);
  while (buffer[*position]) {
    if (islparen(buffer[*position])) {
      return true;
    }
    (*position)++;
  }
  return false;
}
