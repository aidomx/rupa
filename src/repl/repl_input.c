#include <rupa.h>

/**
 * @brief REPL input processing with persistent environment.
 *
 * Flow: check command → lex → check multiline → parse → interpret → flush.
 * The persistent ctx->env keeps variables alive between commands.
 *
 * Multiline handling: when isWaiting is true (open brace/bracket/etc),
 * we APPEND the current line to input->content instead of replacing.
 * This way the lexer processes the full accumulated code on the next
 * iteration when isWaiting becomes false.
 */
void processReplInput(State *state, ReplContext *ctx) {
  if (!state || !ctx) return;

  Buffer *buffer = state->buffer;
  if (!buffer || buffer->length == 0) {
    printf("\r\n");
    fflush(stdout);
    return;
  }

  ctx->env->isRepl = state->isRepl;

  /* Handle REPL dot-commands (.help, .clear, .exit) */
  if (handleReplCommand(state, ctx)) return;

  /* Skip blank input — move to fresh line for next prompt */
  if (isblank(*buffer->value)) {
    printf("\r\n");
    fflush(stdout);
    return;
  }

  /* Save input to history */
  if (!addToHistory(state)) return;

  /* Build input content from the buffer.
   * For multiline: APPEND to existing input (don't replace).
   * For single-line: REPLACE input with just this line. */
  Input *input = state->input;
  char *src = buffer->value;
  size_t len = strlen(src);

  if (input->length > 0) {
    /* Multiline in progress — append to accumulated input */
    if ((int)(input->length + len + 2) >= MAX_BUFFER_SIZE) {
      fprintf(stderr, "Buffer overflow!\n");
      input->length = 0;
      return;
    }
    memcpy(input->content + input->length, src, len);
    input->length += len;
  } else {
    /* Fresh input — copy directly */
    input->cursor = 0;
    if (len + 2 >= MAX_BUFFER_SIZE) {
      fprintf(stderr, "Buffer overflow!\n");
      return;
    }
    memcpy(input->content, src, len);
    input->length = (int)len;
  }

  /* Ensure trailing newline for the lexer */
  if (input->length > 0 && input->content[input->length - 1] != '\n') {
    input->content[input->length++] = '\n';
  }
  input->content[input->length] = '\0';

  /* Lex the full accumulated input */
  lexer(state);

  /* Check if waiting for more input (multiline) */
  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;

  if (flags->isWaiting) {
    /* Multiline in progress — don't execute yet, keep accumulated input */
    setIndent(state->repl);
    editorPushLine(state->repl);
    putchar('\n');
    fflush(stdout);
    return;
  }

  /* Statement is complete — emit newline before output */
  putchar('\n');
  fflush(stdout);

  /* Nothing to execute */
  if (!tokens || tokens->length == 0) {
    input->length = 0;
    resetFlags(flags);
    return;
  }

  /* Parse tokens into AST */
  Request request = createRequest(tokens, 10);
  Node *node = processGenerate(&request);

  if (!node || node->length <= 0) {
    input->length = 0;
    clearStateToken(state->tokens);
    resetFlags(flags);
    return;
  }

  /* Find the PROGRAM root node */
  int root = -1;
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  }

  if (root < 0) {
    input->length = 0;
    clearStateToken(state->tokens);
    resetFlags(flags);
    return;
  }

  /* Interpret with persistent environment — variables survive across commands */
  InterpreterResult result = interpretNode(node, root, ctx->env, ctx->error);

  /* Run event loop to resolve any pending async operations */
  if (result.flow == FLOW_NORMAL && ctx->eventLoop) {
    eventLoopRun(node, ctx->eventLoop, ctx->env, ctx->error);
    eventLoopCleanDone(ctx->eventLoop);
  }

  /* Print errors if any */
  if (result.flow == FLOW_ERROR || (ctx->error && ctx->error->size > 0)) {
    printErrors(ctx->error);
    ctx->error->size = 0;
  }

  /* Flush stdout for REPL mode */
  fflush(stdout);
  /* Statement complete — clear everything for next command */
  input->length = 0;
  if (state->input->length == 0) state->repl->editor->indentLevel = 0;
  clearStateToken(state->tokens);
  resetFlags(flags);
  /* Clear the multiline rollback stack */
  editorClearLineStack(state->repl->editor);
}
