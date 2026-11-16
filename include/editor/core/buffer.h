#pragma once

#if defined(RUPA_PACKAGE_H)

Buffer *createBuffer(int capacity);
void deleteChar(ReplState *repl);
void insertChar(ReplState *repl, char c);

#endif
