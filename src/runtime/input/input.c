#include <rupa.h>

static int findNewStart(History *h, int start, int end) {
  if (!h)
    return 0;

  int currentStart = start;
  for (int i = start; i < end; i++) {
    if (i < h->size - 1 && h->entries[i + 1])
      currentStart = i + 1;
  }

  return currentStart;
}

Input *addToInput(State *state) {
  if (!state)
    return NULL;

  ReplState *repl = state->repl;
  History *h = repl->history;
  Input *input = state->input;

  // re-start by newline
  int start = findNewStart(h, 0, state->size);
  for (int i = start; i < state->size; i++) {
    char *buffer = h->entries[i];
    size_t len = strlen(buffer);

    if (len + input->length + 2 >= MAX_BUFFER_SIZE) {
      fprintf(stderr, "Buffer overflow detection!\n");
      return NULL;
    }

    memcpy(input->content + input->length, buffer, len);
    input->length += len;

    // add newline
    if (input->length > 0 && input->content[input->length - 1] != '\n')
      input->content[input->length++] = '\n';
  }

  // endof string
  if (input->length == 0)
    return NULL;
  input->content[input->length] = '\0';

  return input;
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
