#pragma once

// Standard library
#include "stdlib.h"

// forward (enum, struct)
#include "forward.h"

// core library
#include "core/enum.h"
#include "core/keys.h"
#include "core/limit.h"
#include "core/macros.h"
#include "core/platform.h"

// types
#include "types/compiler/lexer.h"
#include "types/compiler/parse_ast.h"
#include "types/compiler/parse_node.h"
#include "types/editor/editor.h"
#include "types/interpreter/debug.h"
#include "types/interpreter/error.h"
#include "types/repl/repl.h"
#include "types/runtime/context.h"
#include "types/runtime/flags.h"
#include "types/runtime/function.h"
#include "types/runtime/gc.h"
#include "types/runtime/input.h"
#include "types/runtime/io.h"
#include "types/runtime/keyword.h"
#include "types/runtime/validation.h"
#include "types/state/state.h"
#include "types/support/atom.h"
#include "types/support/posix.h"
#include "types/support/symbol.h"
#include "types/support/system.h"

// api
#include "compiler/compiler.h"
#include "editor/editor.h"
#include "interpreter/interpreter.h"
#include "prompt/prompt.h"
#include "repl/repl.h"
#include "runtime/runtime.h"
#include "state/state.h"
// support
#include "utils/atom.h"
#include "utils/identifier.h"
#include "utils/numbers.h"
#include "utils/strings.h"

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
extern void console(CommandType command);

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
void interpreter(Node *node, Error *error);

/** @brief for bootstrap loader
 *
 * @param args from command
 */
int loader(const char *args[], int length);

/**
 * @brief Membaca isi file dan menyimpannya ke buffer.
 *
 * @param path Path file.
 * @param buffer Buffer tujuan.
 * @return True jika berhasil dibaca.
 */
bool readfile(const char *path, Buffer *buffer);
