#pragma once

#include "core/buffer.h"
#include "core/cursor.h"
#include "core/mode.h"

#include "display/drawer.h"
#include "display/refresh.h"
#include "display/terminal.h"

#include "operations/editorState.h"
#include "operations/history.h"
#include "operations/indent.h"
#include "operations/reset.h"

#if defined(RUPA_PACKAGE_H)

extern Editor *createEditor(void);
extern void handleKeyPress(ReplState *repl, int key);

#endif
