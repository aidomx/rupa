#pragma once

#include "annotation.h"
#include "debug/debug.h"
#include "error/error.h"
#include "eval.h"
#include "runtime.h"

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Menjalankan interpreter dari AST node.
 *
 * @param node Root node AST.
 * @param e Struktur error (jika ada).
 */
extern void interpreter(Node *node, Error *error);

extern void interpreterCode(Node *node);

#endif
