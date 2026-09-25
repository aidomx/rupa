#pragma once
#if defined(RUPA_PACKAGE_H)

/* Shared globals and declarations for module loading and dispatch.
 * This is an internal header — only used by loader.c and dispatch.c. */

/* ---- Global state (defined in loader.c) ---- */
extern struct EventLoop *g_event_loop;
extern const char *g_source_file_path;

/* ---- Accessor functions (defined in loader.c) ---- */
void setSourceFilePath(const char *path);
const char *getSourceFilePath(void);
struct EventLoop *getEventLoop(void);

/* ---- Module loading (defined in loader.c) ---- */
/* loadModuleFileError: sama dengan loadModuleFile, plus propagasi
 * ModuleError (mis. circular import) ke `error` bila non-NULL.
 * loadModuleFile adalah wrapper kompat (error = NULL). */
RuntimeValue loadModuleFileError(const char *module_path, bool require_export, Error *error);
RuntimeValue loadModuleFile(const char *module_path, bool require_export);
bool hasDotSlash(const char *path);

/* Dotted path resolve ke FILE leaf sungguhan (bukan index parent, bukan
 * leaf yang di-redirect ke parent)? Menentukan semantik ie.txt #4 vs #5
 * untuk single-entry import. */
bool modSourceResolvesLeaf(const char *module_path);

/* ---- Interpreter dispatch (defined in dispatch.c) ---- */
InterpreterResult interpretNode(Node *node, int id, RuntimeEnv *env, Error *error);

#endif
