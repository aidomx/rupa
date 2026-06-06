#include <rupa.h>

/**
 * @brief Placeholder resolver untuk keyword.
 *
 * Disediakan agar arsitektur tetap extensible
 * meskipun keyword sudah ditangani di layer lain.
 *
 * @param state Pointer ke State
 * @return state
 */
void *resolveKeyword(State *state) {
  if (check_state(state))
    return NULL;
  return state;
}
