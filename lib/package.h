#ifndef PACKAGE_H
#define PACKAGE_H

#include "enum.h"
#include "structure.h"
#include <stdbool.h>

/// ================================
/// Start Comment: Token Utilities
/// ================================

/**
 * @brief Menambahkan delimiter sebagai token.
 *
 * @param token Pointer ke struktur token.
 * @param c Karakter delimiter.
 * @param line Nomor baris.
 * @param row Posisi kolom.
 * @return Status keberhasilan.
 */
int addDelim(Token *token, char c, int line, int row);

/**
 * @brief Menambahkan token baru ke dalam struktur token.
 *
 * @param token Pointer ke struktur token.
 * @param type Tipe token.
 * @param value Nilai string token.
 * @param line Nomor baris.
 * @param row Posisi kolom.
 * @return Status keberhasilan.
 */
int addToken(Token *token, TokenType type, const char *value, int line,
             int row);

/// ==============================
/// End Comment: Token Utilities
/// ==============================

/// ==============================
/// Start Comment: REPL Handling
/// ==============================

/**
 * @brief Menambahkan input pengguna ke dalam history REPL.
 *
 * @param state State REPL aktif.
 * @param input String input.
 */
void addToHistory(ReplState *state, const char *input);

/**
 * @brief Membersihkan semua memori yang digunakan oleh state dan token.
 *
 * @param state Pointer ke ReplState.
 * @param token Pointer ke Token.
 */
void clearAll(ReplState *state, Token *token);

/**
 * @brief Membersihkan layar terminal.
 */
void clearScreen();

/**
 * @brief Menghapus semua riwayat dalam state REPL.
 *
 * @param state Pointer ke ReplState.
 */
void clearState(ReplState *state);

/**
 * @brief Menghapus seluruh token dan mengosongkan buffer.
 *
 * @param token Pointer ke struktur token.
 * @param capacity Kapasitas awal token.
 */
void clearToken(Token *token, int capacity);

/// ==============================
/// End Comment: REPL Handling
/// ==============================

/// ==============================
/// Start Comment: Node & AST
/// ==============================

/**
 * @brief Membuat node AST baru.
 *
 * @param capacity Jumlah maksimum node anak.
 * @return Pointer ke Node yang baru.
 */
Node *createNode(int capacity);

/**
 * @brief Menambahkan node AST ke dalam struktur pohon.
 *
 * @param node Node induk.
 * @param n Node AST baru.
 * @return Status keberhasilan.
 */
int createAst(Node *node, AstNode n);

/// ==============================
/// End Comment: Node & AST
/// ==============================

/// ==============================
/// Start Comment: Token & Compiler
/// ==============================

/**
 * @brief Membuat struktur token baru.
 *
 * @param capacity Jumlah awal kapasitas token.
 * @return Pointer ke Token.
 */
Token *createToken(int capacity);

/**
 * @brief Memulai proses kompilasi berdasarkan konfigurasi.
 *
 * @param cfg Struktur konfigurasi sistem.
 */
void compiler(SystemConfig cfg);

/// ==============================
/// End Comment: Token & Compiler
/// ==============================

/// ==============================
/// Start Comment: REPL Console Loop
/// ==============================

/**
 * @brief Menangani satu siklus interaktif REPL.
 *
 * @param state State REPL saat ini.
 * @param actived Status aktif REPL.
 * @param buffer Buffer input.
 * @param line Nomor baris.
 */
void console(ReplState *state, bool *actived, char buffer[], int line);

/// ==============================
/// End Comment: REPL Console Loop
/// ==============================

/// ==============================
/// Start Comment: Debug & Utilities
/// ==============================

/**
 * @brief Menampilkan struktur AST untuk debugging.
 *
 * @param node Root node AST.
 */
void printAst(Node *node);

/**
 * @brief Mengambil konfigurasi dari baris string berdasarkan key.
 *
 * @param line Baris konfigurasi.
 * @param key Kunci konfigurasi.
 * @param value Buffer output nilai.
 * @return Pointer ke value.
 */
char *getConfig(const char *line, const char *key, char *value);

/**
 * @brief Menangani variabel saat parsing token.
 *
 * @param token Struktur token.
 * @param input Input string.
 * @param line Nomor baris.
 * @param row Nomor kolom.
 */
void handleVariable(Token *token, const char *input, int line, int row);

/**
 * @brief Menjalankan interpreter dari AST node.
 *
 * @param node Root node AST.
 * @param e Struktur error (jika ada).
 */
void interpreter(Node *node, Error *e);

/**
 * @brief Melakukan lexing terhadap string input.
 *
 * @param str Input string.
 * @param t Pointer ke token.
 */
void lexer(char *str, Token *t);

/**
 * @brief Melakukan parsing terhadap token yang sudah ditokenisasi.
 *
 * @param token Struktur token.
 */
void parse(Token *token);

/**
 * @brief Membaca isi file dan menyimpannya ke buffer.
 *
 * @param path Path file.
 * @param buffer Buffer tujuan.
 * @return True jika berhasil dibaca.
 */
bool readfile(const char *path, char *buffer);

/**
 * @brief Menyimpan token dari input tertentu.
 *
 * @param token Pointer token.
 * @param src Sumber string.
 * @param expr Tipe ekspresi.
 * @param line Baris.
 * @param row Kolom.
 */
void saveToken(Token *token, const char *src, char expr, int line, int row);

/// ==============================
/// End Comment: Debug & Utilities
/// ==============================

/// ==============================
/// Start Comment: Entry Point
/// ==============================

/**
 * @brief Memulai Read-Eval-Print Loop (REPL) utama.
 */
void startRepl();

/**
 * @brief Melakukan tokenisasi terhadap input tertentu.
 *
 * @param token Struktur token.
 * @param input Input string.
 * @param line Baris input.
 */
void tokenize(Token *token, const char *input, int line);

/// ==============================
/// End Comment: Entry Point
/// ==============================

#endif
