#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Persistent REPL state — survives across commands.
 *
 * Holds the RuntimeEnv (variables) and EventLoop so state
 * persists from one command to the next, like Node.js/Python REPL.
 */
struct ReplContext {
  struct RuntimeEnv *env;      // Persistent environment
  struct EventLoop *eventLoop; // Persistent event loop
  struct Error *error;         // Persistent error collector
};

extern ReplState *createReplState(int capacity);

/**
 * @brief Memulai Read-Eval-Print Loop (REPL) utama.
 */
extern void startRepl(bool actived);

/**
 * @brief REPL input processing — separate from file-mode processInput.
 *
 * Handles: lex → parse (multiline support) → interpret with persistent env.
 * Does NOT touch history accumulation or processInput flow.
 */
extern void processReplInput(State *state, struct ReplContext *ctx);

/**
 * @brief Handle REPL dot-commands (.help, .clear, .exit).
 *
 * @return true if a command was handled, false if input is not a command.
 */
extern bool handleReplCommand(State *state, struct ReplContext *ctx);

/**
 * @brief Create a ReplContext with persistent env, event loop, error.
 */
extern struct ReplContext *replContextCreate(void);

/**
 * @brief Destroy a ReplContext and free resources.
 */
extern void replContextDestroy(struct ReplContext *ctx);

#endif
