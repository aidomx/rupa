#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * Representasi buffer dinamis berbasis karakter.
 *
 * Buffer digunakan sebagai penyimpanan data mentah yang dapat berubah
 * selama proses input, editing, atau parsing. Struktur ini tidak memiliki
 * pemahaman semantik terhadap isi data — ia hanya mengelola memori dan ukuran.
 */
struct Buffer {
  char *value;  // Pointer ke data karakter
  int length;   // Panjang data yang sedang digunakan
  int capacity; // Kapasitas maksimum buffer
};

/**
 * State editor interaktif untuk REPL.
 *
 * Menyimpan seluruh konteks visual dan logika editing selama REPL berjalan.
 * Editor tidak melakukan parsing atau validasi sintaks.
 */
struct Editor {
  char sequence[8]; // Escape sequence atau input kontrol terminal
  int cursorPos;    // Posisi absolut kursor dalam buffer
  int cursorLine;   // Posisi baris kursor
  int cursorCol;    // Posisi kolom kursor
  int indentLevel;  // Level indentasi aktif
  int lineNumber;   // Nomor baris aktif
  EditorMode mode;  // Mode editor
  EditorAttr attr;  // Atribut editor
};

/**
 * Penyimpanan riwayat input REPL.
 *
 * History menyimpan input-input sebelumnya untuk navigasi ulang.
 */
struct History {
  char **entries;   // Array input sebelumnya
  int capacity;     // Kapasitas maksimum history
  int currentIndex; // Index aktif dalam history
  int size;         // Jumlah entry tersimpan
};

#endif
