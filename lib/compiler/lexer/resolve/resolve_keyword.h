#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Placeholder resolver untuk keyword.
 *
 * Disediakan agar arsitektur tetap extensible
 * meskipun keyword sudah ditangani di layer lain.
 *
 * @param state Pointer ke State
 * @return state
 */
extern void *resolveKeyword(State *state);

#endif
