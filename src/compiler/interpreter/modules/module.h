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

/* ---- Loader split units ----
 * loader.c dipecah menjadi beberapa unit (rules.md: modular). Contract
 * lintas-unit dideklarasikan di sini — header internal yang sama dipakai
 * loader.c, loader_state.c, loader_path.c, dan loader_export.c.
 *
 *   loader_state.c   — state global, guard circular import, module cache
 *   loader_path.c    — utilitas & resolusi path import
 *   loader_export.c  — pembangunan object export entries
 *   loader.c         — entry loadModuleFile/loadModuleFileError */

/* loader_state.c — guard circular import (module stack). */
bool moduleIsLoading(const char *canonical);
void modulePushLoading(char *canonical_owned); /* kepemilikan pindah */
void modulePopLoading(void);

/* loader_state.c — propagasi ModuleError dari error lokal modul. */
void propagateModuleErrors(Error *dst, Error *src);

/* loader_state.c — module cache (memoize hasil load per canonical). */
bool moduleCacheGet(const char *canonical, RuntimeValue *out);
void moduleCachePut(char *canonical_owned, RuntimeValue v); /* kepemilikan pindah */

/* loader_state.c — whole-env snapshot binding module. */
RuntimeValue buildWholeEnvValue(RuntimeEnv *env);

/* loader_path.c — utilitas file & path (prefix mod* untuk memilih
 * dari helper static di TU lain, mis. fileExists di stdlib/loader.c). */
bool modFileExists(const char *path);
char *modResolveModulePath(const char *module_path);
char *modJoinPath(const char *dir, const char *rel);
char *modDirName(const char *path);
char *moduleCanonicalPath(const char *path);        /* GC-managed */
char *resolveDotPath(const char *module_path, const char *source_dir);
char *resolveAbsModulePath(const char *module_path);
char *packageRootOf(const char *file_path);         /* malloc'd */

/* loader_export.c — object entries dari NODE_MOD ExportDecl. */
RuntimeValue modBuildExportEntries(Node *node, int root, const char *module_path, Error *error);

/* Buang isi module cache (lihat definisi di loader.c) — dipakai serve.c
 * agar tiap reload dev server tidak menahan versi lama module hasil
 * import. */
void moduleCacheReset(void);

/* Jumlah entri module cache saat ini (mode `rupa profile --cache`). */
int moduleCacheCount(void);

/* Dotted path resolve ke FILE leaf sungguhan (bukan index parent, bukan
 * leaf yang di-redirect ke parent)? Menentukan semantik ie.txt #4 vs #5
 * untuk single-entry import. */
bool modSourceResolvesLeaf(const char *module_path);

/* ---- Interpreter dispatch (defined in dispatch.c) ---- */
InterpreterResult interpretNode(Node *node, int id, RuntimeEnv *env, Error *error);

#endif
