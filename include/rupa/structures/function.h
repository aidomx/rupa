#pragma once

#include "rupa/package.h"

/**
 * @brief Argument list untuk function calls.
 *
 * Menyimpan value argument dan informasi lokasinya.
 */
struct Args {
  char value[1024];
  int line;
  int row;
  TokenType types;
};

/**
 * @brief Function call representation.
 *
 * Menyimpan nama fungsi, arguments, dan informasi lokasi.
 */
struct Function {
  char func[MAX_VALUE];
  char args[8][256];
  int line;
  int row;
  TokenType types;
};
