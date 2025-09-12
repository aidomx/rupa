#pragma once

#include "rupa/package.h"

/**
 * @brief Informasi detail untuk error reporting.
 *
 * Menyimpan pesan error, kode error, dan lokasi (line, row).
 */
struct ErrorInfo {
  char message[MAX_MESSAGE_LENGTH];
  char code[MAX_CODE_LENGTH];
  int line;
  int row;
  ErrorType type;
};

/**
 * @brief Kumpulan error information.
 *
 * Array untuk menyimpan multiple error information.
 */
struct Error {
  ErrorInfo info[256];
  int length;
};
