#pragma once

// standard C headers
#include "stdlib.h"

// platform layer
#include "core/platform.h"

// forward / core types
#include "forward.h"
#include "core/enum.h"
#include "core/keys.h"
#include "core/limit.h"
#include "core/macros.h"
#include "core/manifest.h"

// api
#include "compiler/compiler.h"
#include "debug/debug.h"
#include "editor/editor.h"
#include "formatter/formatter.h"
#include "prompt/prompt.h"
#include "repl/repl.h"
#include "runtime/runtime.h"
#include "state/state.h"

// Standard library rupa language
#include "modules/rupa_modules.h"

// support
#include "utils/utils.h"

/** @brief for bootstrap loader
 *
 * @param args from command
 */
int loader(const char *args[], int length);

/**
 * @brief Membaca isi file dan menyimpannya ke buffer.
 *
 * @param path Path file.
 * @param buffer Buffer tujuan.
 * @return True jika berhasil dibaca.
 */
bool readfile(const char *path, Buffer *buffer);
