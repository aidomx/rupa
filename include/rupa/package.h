#ifndef RUPA_PACKAGE_H
#define RUPA_PACKAGE_H

#include "rupa/assignment.h"
#include "rupa/ast.h"
#include "rupa/ctypes.h"
#include "rupa/enum.h"
#include "rupa/expression.h"
#include "rupa/limit.h"
#include "rupa/operator.h"
#include "rupa/structure.h"
#include "rupa/token.h"
#include "rupa/utils.h"

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
 */
void clearAll(ReplState *state);

/**
 * @brief Menghapus seluru node dan mengosongkan buffer.
 *
 * @param node pointer node
 */
void clearNode(Node *node);

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

int createBoolean(Node *root, bool value);
int createFloat(Node *root, char *value);

/**
 * @brief Membuat identifier node (variabel) dalam AST.
 *
 * @param root Root node AST.
 * @param name Nama identifier.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createId(Node *root, char *name);

/**
 * @brief Membuat number literal node dalam AST.
 *
 * @param root Root node AST.
 * @param value Nilai integer.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createNumber(Node *root, int value);

int createProgram(Node *root);
int createReturn(Node *root, int expression_id);
int createString(Node *root, char *value, NodeType nodeType);

int createSubscript(Node *root, int posId, int index);

/**
 * @brief Membuat binary operation node dalam AST.
 *
 * @param root Root node AST.
 * @param opToken Operator (+, -, *, /, =, dsb).
 * @param leftId ID node operand kiri.
 * @param rightId ID node operand kanan.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createBinary(Node *root, DataToken *opToken, int leftId, int rightId);

/**
 * @brief Membuat assignment node dalam AST.
 *
 * @param root Root node AST.
 * @param left ID node target (identifier).
 * @param right ID node ekspresi nilai.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createAssignment(Node *root, int left, int right);

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

/// ==============================
/// End Comment: Token & Compiler
/// ==============================

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
/// Start Comment: Parser & AST Builder
/// ==============================

/**
 * @brief Membuat request baru untuk parser.
 *
 * @param tokens Pointer ke struktur token.
 * @param capacity Kapasitas awal node.
 * @return Struktur Request yang terinisialisasi.
 */
Request createRequest(Token *tokens, int capacity);

/**
 * @brief Mem-parse sebuah atom (IDENTIFIER atau NUMBER).
 *
 * @param req Pointer ke Request parser.
 * @param data Pointer ke DataToken.
 * @return Index node hasil parse, atau -1 jika gagal.
 */
int parseAtom(Request *req, DataToken *data);

/**
 * @brief Mem-parse faktor assignment (IDENTIFIER).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Index node identifier, atau -1 jika gagal.
 */
int parseFactor(Request *req, Response res);

/**
 * @brief Mem-parse sebuah ekspresi assignment (kanan "=").
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
Response parseExpression(Request *req, Response res);

/**
 * @brief Mem-parse sebuah statement (assignment).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
Response parseStatement(Request *req, Response res);

/**
 * @brief Loop utama untuk membangun AST dari token list.
 *
 * @param req Pointer ke Request parser.
 * @return Pointer ke Node AST root.
 */
Node *processGenerate(Request *req);

/**
 * @brief Entry point parser: generate AST dari token list.
 *
 * @param tokens Pointer ke Token list.
 */
void generateAst(Token *tokens);

/// ==============================
/// End Comment: Parser & AST Builder
/// ==============================

/// ==============================
/// Start Comment: Debug & Utilities
/// ==============================

/**
 * @brief Proses debugging struktur AST.
 *
 * @param node Root node AST.
 */
void startDebug(Node *node);

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

int parseBinary(Request *req, int start, int end);

/**
 * @brief Membaca isi file dan menyimpannya ke buffer.
 *
 * @param path Path file.
 * @param buffer Buffer tujuan.
 * @return True jika berhasil dibaca.
 */
bool readfile(const char *path, char *buffer);

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
 * @param state ReplState
 */
Token *tokenize(ReplState *state);

/// ==============================
/// End Comment: Entry Point
/// ==============================

#endif
