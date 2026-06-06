#pragma once

#if defined(RUPA_PACKAGE_H)

void moveCursor(ReplState *repl, int direction);
void moveCursorPosition(ReplState *repl, int direction);
void moveCursorToEnd(ReplState *repl);
void moveCursorToStart(ReplState *repl);
void updateCursorLineInfo(ReplState *repl);
void updateEditorCursor(Editor *editor, int newPos);

#endif
