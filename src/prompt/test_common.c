#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * Shared helper: print source lines with line numbers.
 * ================================================================ */
void testPrintSource(const char *src) {
  int line_no = 1;
  printf("  %3d | ", line_no);
  for (int c = 0; src[c]; c++) {
    putchar(src[c]);
    if (src[c] == '\n' && src[c + 1]) {
      line_no++;
      printf("  %3d | ", line_no);
    }
  }
  if (src[strlen(src) - 1] != '\n') putchar('\n');
}

/* ================================================================
 * Shared helper: run lex+parse on a file, return true if valid.
 * ================================================================ */
bool testLexParse(State *state, const char *path, Buffer **outBuf, Token **outTokens,
                  Node **outNode) {
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  /* Reset history so addToInput doesn't skip stale entries. */
  if (state->history) {
    state->history->size = 0;
    state->history->currentIndex = -1;
  }

  Buffer *buffer = state->buffer;
  if (!readfile(path, buffer)) {
    *outBuf = buffer;
    *outTokens = NULL;
    *outNode = NULL;
    return false;
  }

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (state->input->flags && state->input->flags->isWaiting)) {
    *outBuf = buffer;
    *outTokens = tokens;
    *outNode = NULL;
    return false;
  }

  Request req = createRequest(tokens, 10);
  Node *node = processGenerate(&req);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    *outBuf = buffer;
    *outTokens = tokens;
    *outNode = NULL;
    return false;
  }

  *outBuf = buffer;
  *outTokens = tokens;
  *outNode = node;
  return true;
}
