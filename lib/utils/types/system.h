#pragma once
/**
 * @brief System configuration settings.
 *
 * Menyimpan entry point, output format, dan buffer untuk system config.
 */
#if defined(RUPA_PACKAGE_H)
struct SystemConfig {
  char entry[MAX_BUFFER_SIZE];
  char format[64];
  char output[MAX_BUFFER_SIZE];
  int length;
};
#endif
