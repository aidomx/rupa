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
  struct Buffer *buffer;        // Buffer input universal
  struct History *history;      // Riwayat input universal
  struct ReplState *repl;       // State REPL (editor)
  struct StateContext *context; // Konteks eksekusi runtime
  struct Token *tokens;         // Token hasil pemrosesan
  struct Debug *debug;          // Informasi debugging
  int isRepl;                   // Flag mode REPL
  int size;                     // Ukuran state aktif
};

#endif
