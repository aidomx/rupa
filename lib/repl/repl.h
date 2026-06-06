#pragma once

#if defined(RUPA_PACKAGE_H)

extern ReplState *createReplState(int capacity);
/**
 * @brief Memulai Read-Eval-Print Loop (REPL) utama.
 */
extern void startRepl(bool actived);

#endif
