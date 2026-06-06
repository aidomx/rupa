#pragma once
/**
 * @struct GarbageCollector
 * @brief Representasi konteks pengelolaan memori pada runtime.
 *
 * GarbageCollector merepresentasikan *state konseptual* dari kepemilikan
 * memori selama eksekusi program. Setiap alokasi yang relevan terhadap
 * runtime (AST node, token, string, object sementara, dsb.) dapat
 * diregistrasikan ke dalam struktur ini.
 *
 * Struktur ini tidak menentukan *kapan* atau *bagaimana* memori dibebaskan,
 * melainkan hanya menjamin bahwa seluruh alokasi yang terdaftar
 * memiliki titik kontrol yang sama dan dapat diinspeksi oleh sistem
 * (parser, interpreter, semantic analyzer, REPL).
 *
 * Dalam konteks bahasa:
 * - GarbageCollector bertindak sebagai "root" kepemilikan memori
 * - Seluruh fase (lexing → parsing → runtime) dapat berbagi konteks ini
 * - Error, interrupt, atau akhir scope dapat memicu pembersihan terpusat
 *
 * Dengan pendekatan ini, manajemen memori menjadi deterministik,
 * dapat diprediksi, dan mudah di-debug tanpa bergantung pada GC otomatis.
 */
struct GarbageCollector {
  void **items; // Daftar referensi alokasi yang berada dalam konteks runtime
  int capacity; // Batas maksimum referensi yang dapat ditampung
  int count;    // Jumlah referensi aktif dalam konteks ini
};
