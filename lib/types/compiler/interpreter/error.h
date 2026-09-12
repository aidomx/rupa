#pragma once
#if defined(RUPA_PACKAGE_H)
/**
 * @brief Informasi detail untuk error reporting.
 *
 * Menyimpan pesan error, kode error, dan lokasi (line, row).
 */
struct ErrorInfo {
  const char *code;
  char *message;
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
  struct ErrorInfo *info;
  int capacity;
  int size;
};

#endif
