#pragma once
#if defined(RUPA_PACKAGE_H)

/**
 * Union untuk menyimpan nilai debug berdasarkan tipe.
 */
union DebugValue {
  bool boolean;
  int number;
  void *pointer;
  char character;
  size_t size;
};

/**
 * Struktur debug
 *
 * Hanya sebagai penampung agar lebih spesifik
 * bagian mana yang sedang dalam proses debug.
 *
 * dan akan menjadi bagian dari State.
 */
struct Debug {
  char *message;
  bool enabled;
  int line;
  int capacity;
  int length;
  DebugType type;
  union DebugValue value;
  Debug *next;
};

#endif
