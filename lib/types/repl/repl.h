#pragma once
/**
 * Manajemen state REPL secara terpusat.
 *
 * Menggabungkan buffer input, editor, dan history.
 * Hanya aktif saat mode REPL berjalan.
 */
#if defined(RUPA_PACKAGE_H)

struct ReplState {
  int capacity;            // Kapasitas internal REPL
  int size;                // Ukuran data aktif
  struct State *state;      // Back-pointer ke state induk
  struct Buffer *buffer;    // Pointer ke state->buffer (convenience)
  struct Editor *editor;    // State editor
};

#endif
