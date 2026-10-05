#pragma once

#if defined(RUPA_PACKAGE_H)

extern int findLineStart(ReplState *repl);
extern int getCurrentIndent(ReplState *repl);
extern int getOffsetIndent(ReplState *repl);
extern void setIndent(ReplState *repl);

#endif
