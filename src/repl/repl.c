/**
 * @file repl.c
 * @brief Implementasi Read-Eval-Print Loop (REPL) untuk interpreter Rupa.
 *
 * Mencakup pembuatan state, penanganan input pengguna, parsing token,
 * serta kontrol alur utama REPL.
 *
 * @author aidomx
 * @github https://github.com/aidomx/rupa.git
 */
#include <rupa/package.h>

/**
 * @brief Membuat dan menginisialisasi state baru untuk REPL.
 *
 * Fungsi ini mengalokasikan memori untuk struktur ReplState,
 * lalu menyetel nilai awal untuk history, length, dan line.
 *
 * @param capacity integer untuk capacity token.
 * @return Pointer ke ReplState yang sudah dialokasikan.
 */
ReplState *createReplState(int capacity) {
  ReplState *state = malloc(sizeof(ReplState));
  state->tokens = createToken(capacity);
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
 * @param input String input dari pengguna.
 */
void processState(ReplState *state, const char *input) {
  if (!state || !input)
    return;

  if (strcmp(input, ".clear") == 0) {
    clearScreen();
    clearToken(state->tokens, 10);
    clearState(state);
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
 * @param actived Pointer ke flag bool apakah REPL masih aktif.
 * @param buffer Buffer string untuk menyimpan input pengguna.
 */
void editorRepl(ReplState *state, bool *actived, char buffer[]) {
  if (!state) {
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
    processState(state, trimspace(buffer));

    /**
     * Memproses menjadi token jika hanya kondisi
     * panjang dari buffer lebih dari 0.
     */
    if (state->length > 0 && strlen(buffer) > 0) {
      // src/parser/tokenize/main.c
      Token *tokens =
          tokenize(state->tokens, state->history, state->length, state->line);

      if (tokens && tokens->length > 0)
        // src/ast/ast.c
        generateAst(tokens);
    }
  }
}

/**
 * @brief Fungsi utama untuk memulai REPL.
 *
 * Menginisialisasi state dan token, lalu memulai loop interaktif
 * dengan pengguna sampai keluar dari REPL.
 */
void startRepl() {
  ReplState *state = createReplState(10);
  bool actived = true;
  char buffer[MAX_BUFFER_SIZE];

  printf("Welcome to Rupa v%.1f\n", VERSION);

  while (actived) {
    printf("%d ", state->line);
    editorRepl(state, &actived, buffer);
  }

  // Bersihkan memory setelah .exit
  clearAll(state);
}
