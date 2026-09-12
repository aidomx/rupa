/**
 * @brief REPL (Read-Eval-Print Loop) untuk interpreter Rupa.
 *
 * Modular structure:
 * - repl.c         — main REPL loop + context lifecycle
 * - repl_input.c   — processReplInput (persistent env, no history accumulation)
 * - repl_command.c — .help, .clear, .exit handlers
 *
 * @author aidomx
 * @github https://github.com/aidomx/rupa.git
 */
#include <rupa.h>

/* ================================================================
 * ReplContext lifecycle
 * ================================================================ */

ReplContext *replContextCreate(void) {
  ReplContext *ctx = calloc(1, sizeof(ReplContext));
  if (!ctx) return NULL;

  ctx->env = semCreateEnv(NULL);
  if (ctx->env) {
    stdlibInit(ctx->env);
    builtinsInit(ctx->env);
  }

  ctx->eventLoop = eventLoopCreate();
  ctx->error = createError(10);

  return ctx;
}

void replContextDestroy(ReplContext *ctx) {
  if (!ctx) return;

  if (ctx->eventLoop) {
    eventLoopDestroy(ctx->eventLoop);
    ctx->eventLoop = NULL;
  }

  if (ctx->error) {
    /* Error is GC-owned, just clear pointer */
    ctx->error = NULL;
  }

  /* Env bindings are GC-owned */
  ctx->env = NULL;

  free(ctx);
}

/* ================================================================
 * REPL main loop
 * ================================================================ */

void startRepl(bool actived) {
  State *state = createGlobalState(10, actived);

  if (enableRawMode() == -1) {
    printf("Failed to enter raw mode\n");
    return;
  }
  stdIoSetRawMode(true);

  /* Create persistent REPL context (env + event loop) */
  ReplContext *ctx = replContextCreate();

  /* Set global event loop so interpretAsync can push to it */
  extern struct EventLoop *g_event_loop;
  g_event_loop = ctx->eventLoop;

  welcomeMessage();
  ReplState *repl = state->repl;

  /* REPL loop — each iteration handles one keypress */
  while (state->isRepl) {
    /* Update display */
    refreshDisplay(repl);

    /* Read keypress */
    int key = getEditorKey(state);

    /* Exit on Ctrl+D, Ctrl+C, or invalid key */
    state->isRepl = !key || (key == CTRL('D') || key == CTRL('C')) ? false : true;

    if (key == '\r' || key == '\n') {
      /* Enter pressed — process input with persistent env */
      processReplInput(state, ctx);
      /* Always move to new line and reset display.
       * During multiline, indentLevel is preserved by setIndent. */
      resetEditorState(repl);
      /*refreshDisplay(repl);*/
    } else {
      /* Other keys — editor handles cursor, history, insert, etc. */
      handleKeyPress(repl, key);
    }

    /* Reset editor attribute */
    repl->editor->attr = EDITOR_ATTR_NONE;
  }

  /* Cleanup */
  extern struct EventLoop *g_event_loop;
  g_event_loop = NULL;
  replContextDestroy(ctx);
  disableRawMode();
  printf("\r\033[2K");
}
