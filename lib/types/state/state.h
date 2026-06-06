#pragma once
/**
 * State global eksekusi bahasa.
 *
 * Menjadi pusat koordinasi seluruh subsistem:
 * lexer, parser, runtime, REPL, dan debugging.
 */
#if defined(RUPA_PACKAGE_H)

struct State {
  struct Error *error;          // Koleksi error global
  struct Input *input;          // Input aktif
  struct ReplState *repl;       // State REPL (jika aktif)
  struct StateContext *context; // Konteks eksekusi runtime
  struct Token *tokens;         // Token hasil pemrosesan
  struct Debug *debug;          // Informasi debugging
  int isRepl;                   // Flag mode REPL
  int size;                     // Ukuran state aktif
};

#endif
