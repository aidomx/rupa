#pragma once

#if defined(RUPA_PACKAGE_H)

extern void displayEditorUI(Editor *editor);
extern void drawStatusBar(Editor *editor);
extern void drawLineNumbers(Editor *editor);
extern void drawContent(Editor *editor);
extern void drawCommandLine(Editor *editor);
extern void updateCursorPosition(Editor *editor);

#endif
