#pragma once

#include "crypto.h"
#include "datetime.h"
#include "io.h"
#include "net.h"
#include "os.h"
#include "regex.h"
#include "string.h"
#include "test_helper.h"

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

/* ---- spec module (src/stdlib/spec.c) — .spec config untuk user code ----
 * rupa go menyediakan konfigurasi via specModuleProvide(); object
 * bertag __spec (print ditolak — provenance, lihat value.c). */
void specModuleProvide(RuntimeValue module);
void specModuleClear(void);
void specTagObject(RuntimeValue *obj);
RuntimeValue specModuleBuild(const char *host, const char *port, const char *protocol,
                             const char *dbhost, const char *dbport, const char *dbname,
                             const char *dbuser, const char *domain);

/* Initialize built-in functions (type, len, isNull, toNumber, toString) */
void builtinsInit(RuntimeEnv *env);

/* Initialize blok ops contract (ccpy, cmove, cset) */
void rupaMemoryInit(RuntimeEnv *env);

/* sizeof: ukuran representasi tipe (scalar/struct/array "T[]").
 * Return false bila tipe tidak dikenal. */
bool rupaMemorySizeOf(const char *type, int *outSize);

/* View type check handle VALUE_PTR (registry v3 — design/new_memory.txt
 * C4): tipe handle dari gcregtype dicocokkan ke type anotasi; handle
 * tanpa tipe (dupl/dupin) hanya masuk "string"/"ptr". Return false +
 * error bila view type tidak cocok. */
bool memoryHandleTypeCheck(RuntimeValue value, const char *type, Error *error);

/* ===== new/del — sistem memori type-driven (design/new_memory.txt) =====
 * memory.c — di-intercept di interpretCall/interpretSubscript:
 * arg pertama `new` adalah NAMA TIPE (tidak dievaluasi), jadi tidak
 * bisa jadi native function biasa. `new Number()` kapital = tipe;
 * `new number()` lowercase bentrok dengan `x: number`. */
InterpreterResult memoryNewCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error,
                                bool *handled);
/* instance.c — instantiation class `new ClassName(args)` (design/new_class.txt):
 * hook di interpretCall SEBELUM memoryNewCall; mengambil alih hanya bila
 * arg[0] new adalah nama CLASS terdaftar (NODE_CLASS_DECL). Tanpa kaitan
 * manajemen memori — class adalah object runtime, bukan blok memori. */
InterpreterResult instanceNewCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error, bool *handled);
/* object.c — new Object(ref?, init?) (design/object.txt): hook di
 * interpretCall SEBELUM memory/instance hook (callee "new", arg pertama
 * literal "Object"). Instance punya member has/get/set/delete/update/
 * json/text; strict bila kontrak struct (stamp __type / ref distamp). */
InterpreterResult objectNewCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error, bool *handled);
InterpreterResult objectMemberCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                   Error *error, bool *handled);
bool objectIsInstance(RuntimeValue obj);
NativeFn objectMemberFn(const char *name);
bool objectMemberWrite(RuntimeValue inst, const char *key, RuntimeValue val,
                       RuntimeEnv *env, Error *error);

/* input.c — sistem @input class (design/new_class.txt poin 5):
 * marker @input mengaktifkan main.input: Input; handler @input
 * mendeklarasikan strict field yang boleh diinput via input.get(). */
void inputSpecReset(void);
void inputSpecRegister(RuntimeValue spec);
bool inputSpecActive(void);
RuntimeValue inputCreateObject(void);
InterpreterResult memoryDelCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error,
                                bool *handled);
/* x[i] baca/tulis elemen handle (scalar = blok 1 elemen). *handled
 * false bila target bukan VALUE_PTR (biarkan jalur lama). */
InterpreterResult memoryIndexGet(Node *node, int targetId, int indexId, RuntimeEnv *env,
                                 Error *error, bool *handled);
InterpreterResult memoryIndexSet(Node *node, int targetId, int indexId, RuntimeValue val,
                                 RuntimeEnv *env, Error *error, bool *handled);
/* Versi value-based untuk IR machine — lihat deklarasi lanjutan di bawah. */

/* Nama tipe bila node adalah panggilan `new T(...)` (kontrak type
 * permanen: `x = new Number()` mencatat declared type `number`). */
bool memoryNewTypeName(Node *node, int valueId, char *buffer, size_t capacity);

/* `p: T = new Contract(...)` — alokasi type-driven dari anotasi (C1–C4
 * design/new_memory.txt). Return true + *out bila sukses; false +
 * *handled bila node memang Contract tapi alokasi gagal (error sudah
 * ditulis); false + !*handled bila bukan `new Contract`. */
bool memoryContractAssign(Node *node, int valueId, const char *annType, bool norm,
                          RuntimeEnv *env, RuntimeValue *out, Error *error, bool *handled);

/* Versi value-based untuk IR machine (nilai sudah di register).
 * memoryPtrGet return null + *fatal bila akses tidak valid. */
RuntimeValue memoryPtrGet(RuntimeValue ptr, RuntimeValue idx, Error *error, bool *fatal);
bool memoryPtrSet(RuntimeValue ptr, RuntimeValue idx, RuntimeValue val, Error *error,
                  bool *fatal);

/* Member access struct pada handle ptr (C3 — design/new_memory.txt):
 * offset dari layout analyzer + tipe field; struct handle di-resolve
 * dari registry v3. Return false bila handle bukan struct (jalur lama). */
bool memoryMemberGet(void *ptr, const char *field, RuntimeValue *out, Error *error,
                     bool *fatal);
bool memoryMemberSet(void *ptr, const char *field, RuntimeValue val, Error *error,
                     bool *fatal);

/* Write-through string slot (design/str_memory.txt): value string /
 * handle dupl DITULIS ke slot handle Contract string (bukan rebind).
 * Return false bila bukan kasus write-through. */
bool memoryStringSlotWrite(const char *name, void *ptr, RuntimeValue val, Error *error);

/* Read-through string slot: handle Contract string DIBACA sebagai
 * VALUE_STRING dari slot. Return false bila bukan string handle. */
bool memoryStringSlotRead(void *ptr, RuntimeValue *out, Error *error);

/* Get a standard module by name */
bool stdlibGetModule(const char *name, RuntimeValue *out);

/* Module management: add, update, delete, list */
int stdlibManage(const char *args[], int length);

/* Initialize stdlib loader — scan directories and cache module paths */
void stdlibLoaderInit(void);

/* Find a stdlib module by name (returns file path or NULL) */
const char *stdlibFindModule(const char *name);
const char *stdlibFindNamespace(const char *name);

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
