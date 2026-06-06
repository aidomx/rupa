#pragma once

#if defined(RUPA_PACKAGE_H)

extern void disableRawMode(void);
extern int enableRawMode(void);
extern int getEditorKey(State *state);
extern int getWindowSize(int *rows, int *cols);
extern int getWindowSizeFallback(int *rows, int *cols);
extern int getCusorPosition(int *rows, int *cols);

#endif
