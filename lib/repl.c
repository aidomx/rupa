/**
 * @file repl.c
 * @brief Implementasi Read-Eval-Print Loop (REPL) untuk interpreter Rupa.
 *
 * Mencakup pembuatan state, penanganan input pengguna, parsing token,
 * serta kontrol alur utama REPL.
 */

#include "enum.h"
#include "limit.h"
#include "package.h"
#include "structure.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Membuat dan menginisialisasi state baru untuk REPL.
 *
 * Fungsi ini mengalokasikan memori untuk struktur ReplState,
 * lalu menyetel nilai awal untuk history, length, dan line.
 *
 * @return Pointer ke ReplState yang sudah dialokasikan.
 */
ReplState *createState() {
  ReplState *state = malloc(sizeof(ReplState));
  state->history = NULL;
  state->length = 0;
  state->line = 1;
  return state;
}

/**
 * @brief Memproses input dari pengguna ke dalam state REPL.
 *
 * Jika input adalah perintah `.clear`, maka layar, state, dan token
 * dibersihkan. Jika bukan, maka input ditambahkan ke history dan baris
 * ditingkatkan.
 *
 * @param state Pointer ke struktur ReplState yang aktif.
 * @param token Pointer ke struktur Token untuk parsing input.
 * @param input String input dari pengguna.
 */
void processState(ReplState *state, Token *token, const char *input) {
  if (!state || !token || !input)
    return;

  if (strcmp(input, ".clear") == 0) {
    clearScreen();
    clearState(state);
    clearToken(token, 10);
  } else {
    addToHistory(state, input);
    state->line++;
  }
}

/**
 * @brief Menangani satu iterasi input dari pengguna dalam mode REPL.
 *
 * Fungsi ini membaca input dari stdin, mengecek apakah pengguna ingin keluar
 * (.exit), lalu memproses input dan melakukan tokenisasi serta parsing.
 *
 * @param state Pointer ke REPL state saat ini.
 * @param token Pointer ke token yang akan dipakai untuk parsing.
 * @param actived Pointer ke flag bool apakah REPL masih aktif.
 * @param buffer Buffer string untuk menyimpan input pengguna.
 * @param line Nomor baris saat ini.
 */
void editorRepl(ReplState *state, Token *token, bool *actived, char buffer[],
                int line) {
  if (!state || !token) {
    *actived = false;
    return;
  }

  if (fgets(buffer, MAX_BUFFER_SIZE, stdin) == NULL) {
    *actived = false;
    return;
  }

  buffer[strcspn(buffer, "\n")] = '\0';

  if (strcmp(buffer, ".exit") == 0) {
    *actived = false;
  } else {
    processState(state, token, buffer);
    tokenize(token, buffer, state->line);
    parse(token);
  }
}

/**
 * @brief Fungsi utama untuk memulai REPL.
 *
 * Menginisialisasi state dan token, lalu memulai loop interaktif
 * dengan pengguna sampai keluar dari REPL.
 */
void startRepl() {
  ReplState *state = createState();
  Token *token = createToken(10);
  bool actived = true;
  char buffer[MAX_BUFFER_SIZE];

  printf("Welcome to Rupa v%.1f\n", VERSION);

  while (actived) {
    printf("%d ", state->line);
    editorRepl(state, token, &actived, buffer, state->line);
  }

  clearAll(state, token);
}
