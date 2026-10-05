#pragma once

#if defined(RUPA_PACKAGE_H)
/**
 * @brief Menambahkan input pengguna ke dalam history REPL.
 *
 * @param state State REPL aktif.
 */
extern History *addToHistory(State *state);
extern History *createHistory(int capacity);
extern void navigateHistory(ReplState *repl, int direction);

#endif
