#pragma once
#include "node_type.h"

#if defined(RUPA_PACKAGE_H)

void addToProgram(struct Node *node, int programId, int declId);

/**
 * @brief Membuat node AST baru.
 *
 * @param capacity Jumlah maksimum node anak.
 * @return Pointer ke Node yang baru.
 */
struct Node *createNode(int capacity);

int createComment(struct Node *root, const char *value, int type);

int createArray(struct Node *root, int *elements, int length);
/**
 * @brief Membuat node untuk tipe array postfix, misalnya `number[]`.
 */
int createArrayType(struct Node *root, int elementType);

/**
 * @brief Membuat node tipe dari nama scalar atau postfix array berulang.
 */
int createTypeNode(struct Node *root, const char *typeName);

/**
 * @brief Membuat object literal node dalam AST (mis. `{ key: value, ... }`).
 *
 * @param root Root node AST.
 * @param entries Array pasangan key-value node id.
 * @param length Jumlah entries.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createObject(struct Node *root, struct AstObjectEntry *entries, int length);

/**
 * @brief Menambahkan node AST ke dalam struktur pohon.
 *
 * @param node Node induk.
 * @param n Node AST baru.
 * @return Status keberhasilan.
 */
int createAst(struct Node *node, struct AstNode n);
int createBoolean(struct Node *root, bool value);
int createDecimal(struct Node *root, char *value);

/**
 * @brief Membuat identifier node (variabel) dalam AST.
 *
 * @param root Root node AST.
 * @param name Nama identifier.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createId(struct Node *root, char *name);

/**
 * @brief Membuat number literal node dalam AST.
 *
 * @param root Root node AST.
 * @param value Nilai integer (64-bit).
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createNumber(struct Node *root, long long value);

int createProgram(struct Node *root);
int createReturn(struct Node *root, int expression_id);
int createExpressionStatement(struct Node *root, int expression_id);
int createBreak(struct Node *root);
int createContinue(struct Node *root);
int createUpdate(struct Node *root, int target, const char *op, bool prefix, int value);
int createString(struct Node *root, char *value, enum NodeType nodeType);

int createSubscript(struct Node *root, int posId, int index);

/**
 * @brief Membuat binary operation node dalam AST.
 *
 * @param root Root node AST.
 * @param opToken Operator (+, -, *, /, =, dsb).
 * @param leftId ID node operand kiri.
 * @param rightId ID node operand kanan.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createBinary(struct Node *root, struct DataToken *opToken, int leftId, int rightId);
int createNot(struct Node *root, int operandId);

/**
 * @brief Membuat assignment node dalam AST.
 *
 * @param root Root node AST.
 * @param left ID node target (identifier).
 * @param right ID node ekspresi nilai.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createAssignment(struct Node *root, int left, int type, int right);
int createAssignmentConst(struct Node *root, int left, int type, int right, bool isConst);
int createConditionalAssignment(struct Node *root, int target, int value);
int createThen(struct Node *root, int condition, int result);
int createFallback(struct Node *root, int primary, int fallback);
int createAsync(struct Node *root, int request, int handler, int timeout, int loaderId,
                int timeoutId);
int createAwait(struct Node *root, int expression);
int createMember(struct Node *root, int object, int member);
int createCase(struct Node *root, int subject, struct AstCaseEntry *entries, int length);
int createMemberAssign(struct Node *root, int target, int value);
int createStringInterp(struct Node *root, int *parts, int length);
int createCall(struct Node *root, int callee, int *args, int length);
int createPrint(struct Node *root, int *args, int length);
int createBlock(struct Node *root, int *items, int length);
int createIf(struct Node *root, int condition, int thenBlock, int elseBlock, bool isBlock);
int createLoop(struct Node *root, const char *kind, int condition, int body);
int createFunctionDecl(struct Node *root, int name, int *params, int paramLength, int body,
                       int returnType);
int createStructDecl(struct Node *root, int name, int body);
int createEnumDecl(struct Node *root, int name, int body);
int createClassDecl(struct Node *root, int name, int typeId, int body);
int createClassDeclExt(struct Node *root, int name, int typeId, int body, int parent);
int createMarker(struct Node *root, int name, int value);
int createAnnotation(struct Node *root, int name, int type, int value);
int createModule(struct Node *root, enum NodeType type, int value, int name);

/* ---- Module design (NODE_MOD) — 1 container untuk import & export ----
 * Entry helpers — semua alokasi via GC (gccalloc/gcstrdup), tanpa free manual.
 */
AstModEntry *modEntry(const char *name);
AstModEntry *modEntryKey(const char *name, const char *key);
AstModEntry *modEntryWild(const char *name);
AstModEntry *modEntryMember(const char *name, AstModEntry *path);
AstModEntry *modPolicy(const char *name, const char *value);

/**
 * Membuat NODE_MOD dengan type ImportDecl.
 *
 * @param root Root node AST.
 * @param entries Array pointer entry yang diimport (di-copy ke GC).
 * @param entryCount Jumlah entries.
 * @param source Path module ("../modules", "rupa.os"), atau NULL.
 * @param sourceAlias Alias `from X as m`, atau NULL.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createModImport(struct Node *root, AstModEntry **entries, int entryCount, const char *source,
                    const char *sourceAlias);

/**
 * Membuat NODE_MOD dengan type ExportDecl.
 *
 * @param root Root node AST.
 * @param entries Array pointer entry yang diexport (di-copy ke GC).
 * @param entryCount Jumlah entries.
 * @param source Path module, atau NULL untuk export lokal (`export x`).
 * @param policies Array pointer policy `{ a: private }`, atau NULL.
 * @param policyCount Jumlah policies.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createModExport(struct Node *root, AstModEntry **entries, int entryCount, const char *source,
                    AstModEntry **policies, int policyCount);

/**
 * Membuat NODE_MOD dengan type NamespaceDecl (`namespace db { export ... }`).
 *
 * @param root Root node AST.
 * @param name Nama namespace (bind target, mis. "db").
 * @param body ID node NODE_BLOCK berisi ExportDecl bersarang.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createModNamespace(struct Node *root, const char *name, int body);

/**
 * @brief Membuat request baru untuk parser.
 *
 * @param tokens Pointer ke struktur token.
 * @param capacity Kapasitas awal node.
 * @return Struktur Request yang terinisialisasi.
 */
struct Request createRequest(struct Token *tokens, int capacity);
Request createRequestWithError(struct Token *tokens, int capacity, struct Error *error);

/**
 * @brief Menghapus seluruh node dan mengosongkan buffer.
 *
 * @param node pointer node
 */
void clearNode(struct Node *node);

extern DataToken *g_parser_token;

#endif
