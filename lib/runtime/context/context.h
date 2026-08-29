#pragma once

#if defined(RUPA_PACKAGE_H)

StateContext *createStateContext(int capacity);

void clearStateContext(StateContext *ctx);
extern void setContextInput(State *state);

extern void processContextInput(State *state);

#endif
