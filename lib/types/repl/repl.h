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
  struct Buffer *buffer;   // Buffer input
  struct Editor *editor;   // State editor
  struct History *history; // Riwayat input
};

#endif
