#ifndef RUPA_PACKAGE_H
#define RUPA_PACKAGE_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rupa/core/package.h>

// forward declarations struct
typedef struct Token Token;
typedef struct DataToken DataToken;
typedef struct Node Node;
typedef struct Array Array;
typedef struct AstNode AstNode;
typedef struct ReplState ReplState;
typedef struct Request Request;
typedef struct Response Response;
typedef struct SystemConfig SystemConfig;
typedef struct Error Error;
typedef struct State State;

// forward struct Ast*
typedef struct AstArray AstArray;
typedef struct AstAssignment AstAssignment;
typedef struct AstBinary AstBinary;
typedef struct AstBinaryExpression AstBinaryExpression;
typedef struct AstBoolean AstBoolean;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstDouble AstDouble;
typedef struct AstFloat AstFloat;
typedef struct AstIdentifier AstIdentifier;
typedef struct AstNumber AstNumber;
typedef struct AstProgram AstProgram;
typedef struct AstReturn AstReturn;
typedef struct AstString AstString;
typedef struct AstSubscript AstSubscript;
typedef struct AstVariable AstVariable;
typedef struct Assignment Assignment;
typedef struct ErrorInfo ErrorInfo;
typedef struct SymbolTable SymbolTable;
typedef struct SymbolToken SymbolToken;
typedef struct Args Args;
typedef struct Identifier Identifier;
typedef struct Function Function;
typedef struct Number Number;
typedef struct Binary Binary;
typedef struct Position Position;

// ==================== ENUM DECLARATIONS ====================
typedef enum TokenType TokenType;
typedef enum NodeType NodeType;
typedef enum ErrorType ErrorType;
typedef enum BinaryType BinaryType;
typedef enum VariableType VariableType;

#include "rupa/structures/posix.h"

#include "rupa/ast/package.h"
#include "rupa/repl/package.h"
#include "rupa/structures/package.h"
#include "rupa/tokenize/package.h"
#include "rupa/utils/package.h"

/**
 * @brief Membersihkan layar terminal.
 */
void clearScreen();

/**
 * @brief Memulai proses kompilasi berdasarkan konfigurasi.
 *
 * @param cfg Struktur konfigurasi sistem.
 */
void compiler(SystemConfig cfg);

/**
 * @brief Menangani satu siklus interaktif REPL.
 *
 * @param state State REPL saat ini.
 * @param actived Status aktif REPL.
 * @param buffer Buffer input.
 * @param line Nomor baris.
 */
void console(ReplState *state, bool *actived, char buffer[], int line);

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
 * @brief Membaca isi file dan menyimpannya ke buffer.
 *
 * @param path Path file.
 * @param buffer Buffer tujuan.
 * @return True jika berhasil dibaca.
 */
bool readfile(const char *path, char *buffer);

#endif
