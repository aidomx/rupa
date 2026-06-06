#pragma once
/**
 * Definisi tipe pada saat proses interpreter berjalan,
 * untuk daftar ErrorType dibantu oleh ChatGPT.
 *
 * @ErrorType
 * @reference ChatGPT
 */
enum ErrorType {
  ERROR_NONE,
  ERR,  // Kesalahan umum / tidak diketahui
  WARN, // Peringatan umum
  ERR_INVALID_FUNCTION_NAME,
  // Tokenizer / Lexer errors
  ERR_UNEXPECTED_CHAR,  // Karakter tidak dikenali
  ERR_UNTERMINATED_STR, // String tidak ditutup
  ERR_INVALID_NUMBER,   // Format angka tidak valid

  // Parser errors
  ERR_SYNTAX,           // Kesalahan sintaks umum
  ERR_MISSING_TOKEN,    // Token penting hilang (mis. '}' atau ')')
  ERR_UNEXPECTED_TOKEN, // Token tidak sesuai konteks
  ERR_UNEXPECTED_EOF,   // Akhir file tidak terduga saat parsing

  // Semantic errors
  ERR_UNDEFINED_VAR,  // Variabel belum dideklarasikan
  ERR_REDECLARED_VAR, // Variabel sudah dideklarasikan
  ERR_TYPE_MISMATCH,  // Tipe tidak sesuai (mis. string ditambah int)
  ERR_INVALID_ASSIGN, // Penugasan ke konstanta atau rvalue

  // Runtime errors
  ERR_DIV_BY_ZERO,         // Pembagian oleh nol
  ERR_NULL_DEREF,          // Mengakses nilai null
  ERR_INDEX_OUT_OF_BOUNDS, // Akses indeks di luar batas
  ERR_INVALID_CALL,        // Pemanggilan fungsi pada non-fungsi
  ERR_STACK_OVERFLOW,      // Tumpukan melebihi batas

  // Warning types
  WARN_UNUSED_VAR,    // Variabel tidak digunakan
  WARN_DEPRECATED,    // Penggunaan fitur usang
  WARN_SHADOWING,     // Variabel menimpa variabel di scope luar
  WARN_POSSIBLE_NULL, // Potensi akses null

  // Internal/engine errors
  ERR_INTERNAL,       // Kesalahan internal interpreter
  ERR_NOT_IMPLEMENTED // Fitur belum diimplementasi
};
