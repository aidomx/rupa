/**
 * @brief Implementasi Read-Eval-Print Loop (REPL) untuk interpreter Rupa.
 *
 * Mencakup pembuatan state, penanganan input pengguna, parsing token,
 * serta kontrol alur utama REPL.
 *
 * @author aidomx
 * @github https://github.com/aidomx/rupa.git
 */
#include <rupa.h>

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
void editorRepl(State *state) {
  if (!state)
    return;

  ReplState *repl = state->repl;

  if (!repl)
    return;

  /*if (fgets(repl->buffer, MAX_BUFFER_SIZE, stdin) == NULL)*/
  /*return;*/

  /*char *newline = strchr(repl->buffer, '\n');*/
  /*repl->indentPos = newline ? newline - repl->buffer : -1;*/

  /*repl->buffer[strcspn(repl->buffer, "\n")] = '\0';*/

  /*if (strcmp(repl->buffer, ".clear") == 0) {*/
  /*clearScreen();*/
  /*clearReplState(repl);*/
  /*welcomeMessage();*/
  /*return;*/
  /*}*/

  /*if (strcmp(repl->buffer, ".exit") == 0) {*/
  /*state->isRepl = false;*/
  /*return;*/
  /*}*/

  /*processReplState(repl, trimspace(repl->buffer));*/
  /**
   * Memproses menjadi token jika hanya kondisi
   * panjang dari buffer lebih dari 0.
   */

  /*if (state->length > 0 && strlen(state->input) > 0) {*/
  /*// src/parser/tokenize/main.c*/
  /*Token *tokens =*/
  /*tokenize(state->tokens, state->history, state->length, state->line);*/

  /*if (tokens && tokens->length > 0)*/
  /*// src/ast/ast.c*/
  /*generateAst(tokens);*/
  /*}*/
}

void processInput(State *state) {
  if (!state)
    return;

  printf("\n");
  ReplState *repl = state->repl;
  Buffer *buffer = repl->buffer;

  if (buffer->length == 0 || isblank(*buffer->value))
    return;

  if (strcmp(buffer->value, ".help") == 0) {
    help(true);
    return;
  }

  if (strcmp(buffer->value, ".clear") == 0) {
    clearScreen();
    welcomeMessage();
    clearInput(state->input);
    clearReplState(repl);
    clearStateToken(state->tokens);
    return;
  }

  if (strcmp(buffer->value, ".exit") == 0) {
    state->isRepl = false;
    clearInput(state->input);
    clearReplState(repl);
    clearStateToken(state->tokens);
    return;
  }

  if (buffer->value[0] == '.')
    return;

  setIndent(repl);
  // Jika gagal menyimpan pada history hentikan
  if (!addToHistory(state))
    return;
  // Hentikan jika transfer history pada input besar
  // mengalami kegagalan saat proses transmisi
  if (!addToInput(state))
    return;

  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;

  printf("token length: %d\n", tokens->length);

  if (!flags->isWaiting && (tokens && tokens->length > 0))
    generateAst(tokens);

  resetFlags(flags);
  // atur konteks dengan membaca hasil input.
  // processContextInput(state);
  // setContextInput(state);
  // printf("[DEBUG]: %s\n", buf->value);
}

/**
 * @brief Fungsi utama untuk memulai REPL.
 *
 * @param pointer GarbageCollector *gc
 * @param bool actived
 */
void startRepl(bool actived) {
  State *state = createGlobalState(10, actived);

  if (enableRawMode() == -1) {
    printf("Failed to enter raw mode\n");
    return;
  }

  welcomeMessage();
  ReplState *repl = state->repl;

  // looping akan berhenti pada kondisi false
  while (state->isRepl) {
    // selalu perbarui tampilan jika ada perubahan
    refreshDisplay(repl);
    // dapatkan tombol keyboard
    int key = getEditorKey(state);
    // keluarkan jika tidak valid atau gagal
    // atau menggunakan tombol kombinasi untuk keluar
    state->isRepl =
        !key || (key == CTRL('D') || key == CTRL('C')) ? false : true;

    // tombol enter
    if (key == '\r' || key == '\n') {
      // proses input saat enter
      processInput(state);
      resetEditorState(repl);
    }
    // untuk key yang lain misal tombol arrow
    // file: src/editor/editor.c
    else
      handleKeyPress(repl, key);

    // reset: none untuk attribute dari editor
    repl->editor->attr = EDITOR_ATTR_NONE;
  }

  disableRawMode();
  // Bersihkan layar saat keluar
  printf("\r\033[2K");
}
