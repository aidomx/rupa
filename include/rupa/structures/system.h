#pragma once

#include "rupa/package.h"

/**
 * @brief System configuration settings.
 *
 * Menyimpan entry point, output format, dan buffer untuk system config.
 */
struct SystemConfig {
  char entry[MAX_BUFFER_SIZE];
  char format[64];
  char output[MAX_BUFFER_SIZE];
  int length;
};
