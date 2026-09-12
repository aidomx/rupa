#pragma once

#include "../stdlib/io.h"
#include "../stdlib/os.h"
#include "../stdlib/string.h"

/* ---- HTTP Module (shared between http_server.c and http_client.c) ---- */
#define MAX_SERVERS 8
#define MAX_REQUEST_SIZE 8192
#define MAX_RESPONSE_SIZE 65536
#define _XOPEN_SOURCE 700

#if defined(RUPA_PACKAGE_H)

typedef struct {
  int server_fd;
  int port;
  bool running;
  pthread_t thread;
  RuntimeValue handler;
  bool has_handler;
} ServerEntry;

extern ServerEntry serverTable[MAX_SERVERS];
extern int serverCount;
extern pthread_mutex_t serverMutex;

InterpreterResult httpRequest(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);
InterpreterResult httpServer(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);
InterpreterResult httpStop(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);

/* ---- AST Factory (shared between factory.c and factory_nodes.c) ---- */
int *copyIds(const int *ids, int length);

/* ---- JSON Parser (shared between json.c and json_parser.c) ---- */
InterpreterResult jsonStringify(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);

typedef struct {
  const char *text;
  size_t position;
} JsonParser;

void skipJsonWhitespace(JsonParser *parser);
bool consume(JsonParser *parser, char expected);
RuntimeValue parseJsonValue(JsonParser *parser, bool *ok);
RuntimeValue parseJsonString(JsonParser *parser, bool *ok);
RuntimeValue parseJsonNumber(JsonParser *parser, bool *ok);
RuntimeValue parseJsonArray(JsonParser *parser, bool *ok);
RuntimeValue parseJsonObject(JsonParser *parser, bool *ok);

/* Initialize all standard modules and register them in the environment */
void stdlibInit(RuntimeEnv *env);

/* Initialize built-in functions (type, len, isNull, toNumber, toString) */
void builtinsInit(RuntimeEnv *env);

/* Get a standard module by name */
bool stdlibGetModule(const char *name, RuntimeValue *out);

/* Module management: add, update, delete, list */
int stdlibManage(const char *args[], int length);

/* Initialize stdlib loader — scan directories and cache module paths */
void stdlibLoaderInit(void);

/* Find a stdlib module by name (returns file path or NULL) */
const char *stdlibFindModule(const char *name);

/* Get the count of cached stdlib modules */
int stdlibModuleCount(void);

/* Refresh stdlib cache after adding/removing packages */
void stdlibLoaderRefresh(void);

/* Get the path to ~/.rupa/rupa_modules.tar.gz (global archive)
 * Caller must free() the returned string. */
char *getGlobalArchive(void);

/* Cleanup extracted stdlib directory */
void stdlibLoaderCleanup(void);

/* Manifest operations */
int manifestAddPackage(const char *sourcePath, const char *packageName, const char *version,
                       const char *description);
int manifestRemovePackage(const char *packageName);
int manifestListPackages(void);

#endif
