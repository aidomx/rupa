#pragma once
#if defined(RUPA_PACKAGE_H)

const char *getAttrString(EditorAttr atrr);
const char *getModeString(EditorMode mode);
void setEditorAttr(Editor *editor, EditorAttr attr);
void setEditorMode(Editor *editor, EditorMode mode);

#endif
