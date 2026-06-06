#include <rupa.h>

const char *getAttrString(EditorAttr attr) {
  switch (attr) {
  case EDITOR_ATTR_ARROW_UP:
    return "ARROW_UP";
  case EDITOR_ATTR_ARROW_DOWN:
    return "ARROW_DOWN";
  case EDITOR_ATTR_ARROW_RIGHT:
    return "ARROW_RIGHT";
  case EDITOR_ATTR_ARROW_LEFT:
    return "ARROW_LEFT";
  case EDITOR_ATTR_HOME:
    return "HOME";
  case EDITOR_ATTR_END:
    return "END";
  case EDITOR_ATTR_DELETE:
    return "DELETE";
  case EDITOR_ATTR_BACKSPACE:
    return "BACKSPACE";
  case EDITOR_ATTR_ESCAPE:
    return "ESCAPE";
  case EDITOR_ATTR_NONE:
  default:
    return "NONE";
  }
}

const char *getModeString(EditorMode mode) {
  switch (mode) {
  case EDITOR_MODE_COMMAND:
    return "COMMAND";
  case EDITOR_MODE_INSERT:
    return "INSERT";
  case EDITOR_MODE_NORMAL:
    return "NORMAL";
  default:
    return "UNKNOWN";
  }
}

void setEditorAttr(Editor *editor, EditorAttr attr) {
  if (!editor)
    return;
  editor->attr = attr;
}

void setEditorMode(Editor *editor, EditorMode mode) {
  if (!editor)
    return;
  editor->mode = mode;
  editor->attr = EDITOR_ATTR_NONE;
}
