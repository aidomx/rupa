#pragma once

#if defined(RUPA_PACKAGE_H)

extern Input *addToInput(State *state);
extern Input *createInput(int capacity);
extern StateInput *createStateInput(void);
extern void clearInput(Input *input);
extern void processInput(State *state);

#endif
