#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Entry point lexer untuk satu unit program.
 *
 * Fungsi ini:
 * - menginisialisasi atom dan lexer state
 * - menentukan primary atom
 * - menyelesaikan token atau masuk mode waiting
 *
 * @param state Pointer ke State
 * @return state atau NULL jika invalid
 */
extern void *resolveProgram(State *state);

#endif
