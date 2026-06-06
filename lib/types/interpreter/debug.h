#pragma once
#if defined(RUPA_PACKAGE_H)
/**
 * Struktur debug
 *
 * Hanya sebagai penampung agar lebih spesifik
 * bagian mana yang sedang dalam proses debug.
 *
 * dan akan menjadi bagian dari State.
 */
struct Debug {
  // dengan all berarti proses debug berlaku
  // untuk semuanya selama proses debugging.
  bool all;
  bool ast;
  bool buffer;
  bool editor;
  bool input;
  bool interpreter;
  bool context;
  bool repl;
  bool tokenize;
};

#endif
