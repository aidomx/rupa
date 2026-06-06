#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Validasi state utama sebelum lexer berjalan.
 * @param state Pointer ke State
 * @return true jika state tidak valid
 */
extern bool check_state(State *state);
extern void clearScreen();
extern State *createGlobalState(int capacity, bool actived);

/**
 * @brief Membersihkan semua memori yang digunakan oleh state dan token.
 *
 * @param state Pointer ke ReplState.
 */
extern void clearAll(ReplState *state);

/**
 * @brief Menghapus semua riwayat dalam state REPL.
 *
 * @param state Pointer ke ReplState.
 */
extern void clearState(ReplState *state);
extern void clearReplState(ReplState *repl);
extern void clearStateContext(StateContext *ctx);
extern void clearStateToken(Token *token);
extern void clearGlobalState(State *state, int capacity);

#endif
