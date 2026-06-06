#pragma once

#if defined(RUPA_PACKAGE_H)

Context *createContext(size_t size);
StateContext *createStateContext(int capacity);

void clearStateContext(StateContext *ctx);
extern void setContextInput(State *state);

extern void processContextInput(State *state);

#endif
