#pragma once

#include "annotation.h"
#include "debug/debug_ast.h"
#include "error/error.h"
#include "eval.h"
#include "function.h"
#include "result.h"
#include "runtime.h"
#include "statement/loop.h"
#include "text.h"

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Menjalankan interpreter dari AST node.
 *
 * @param node Root node AST.
 * @param e Struktur error (jika ada).
 */
extern void interpreter(Node *node, Error *error);

extern void interpreterCode(Node *node);

/**
 * @brief Set the source file path for resolving relative imports.
 */
extern void setSourceFilePath(const char *path);

/**
 * @brief Get the current source file path.
 */
extern const char *getSourceFilePath(void);

/**
 * @brief Get the entry-point script path (main file being run).
 *
 * Tidak berubah saat loader mengganti g_source_file_path untuk
 * modul yang sedang dieksekusi — selalu file yang dijalankan user.
 */
extern const char *getMainScriptPath(void);

#endif
