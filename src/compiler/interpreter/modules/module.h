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
RuntimeValue loadModuleFile(const char *module_path, bool require_export);
bool hasDotSlash(const char *path);

/* ---- Interpreter dispatch (defined in dispatch.c) ---- */
InterpreterResult interpretNode(Node *node, int id, RuntimeEnv *env, Error *error);

#endif
