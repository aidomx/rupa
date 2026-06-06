#pragma once

#if defined(RUPA_PACKAGE_H)

extern void handleCommandMode(ReplState *repl, int key);
extern void handleInsertMode(ReplState *repl, int key);
extern void handleNormalMode(ReplState *repl, int key);

#endif
