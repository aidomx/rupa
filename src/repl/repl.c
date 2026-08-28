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
    repl->editor->lineNumber = 0;
    /*clearInput(state->input);*/
    /*clearReplState(repl);*/
    /*clearStateToken(state->tokens);*/
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

  if (!flags->isWaiting && (tokens && tokens->length > 0))
    generateAst(tokens);

  resetFlags(flags);
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
