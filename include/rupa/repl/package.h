#pragma once

#include "rupa/package.h"

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
 * @brief Menghapus semua riwayat dalam state REPL.
 *
 * @param state Pointer ke ReplState.
 */
void clearState(ReplState *state);

/**
 * @brief Memulai Read-Eval-Print Loop (REPL) utama.
 */
void startRepl();
