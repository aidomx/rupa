#pragma once

#if defined(RUPA_PACKAGE_H)

void addToProgram(struct Node *node, int programId, int declId);

/**
 * @brief Membuat node AST baru.
 *
 * @param capacity Jumlah maksimum node anak.
 * @return Pointer ke Node yang baru.
 */
struct Node *createNode(int capacity);

int createArray(struct Node *root, int *elements, int length);

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
 * @param value Nilai integer.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createNumber(struct Node *root, int value);

int createProgram(struct Node *root);
int createReturn(struct Node *root, int expression_id);
int createBreak(struct Node *root);
int createContinue(struct Node *root);
int createUpdate(struct Node *root, int target, const char *op, bool prefix);
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
int createBinary(struct Node *root, struct DataToken *opToken, int leftId,
                 int rightId);

/**
 * @brief Membuat assignment node dalam AST.
 *
 * @param root Root node AST.
 * @param left ID node target (identifier).
 * @param right ID node ekspresi nilai.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createAssignment(struct Node *root, int left, int type, int right);
int createConditionalAssignment(struct Node *root, int target, int value);
int createThen(struct Node *root, int condition, int result);
int createFallback(struct Node *root, int primary, int fallback);
int createAsync(struct Node *root, int request, int handler, int timeout);
int createAwait(struct Node *root, int expression);
int createMember(struct Node *root, int object, int member);
int createCase(struct Node *root, int subject, struct AstCaseEntry *entries, int length);
int createMemberAssign(struct Node *root, int target, int value);
int createCall(struct Node *root, int callee, int *args, int length);
int createPrint(struct Node *root, int *args, int length);
int createBlock(struct Node *root, int *items, int length);
int createIf(struct Node *root, int condition, int thenBlock, int elseBlock);
int createLoop(struct Node *root, const char *kind, int condition, int body);
int createFunctionDecl(struct Node *root, int name, int *params, int paramLength, int body);
int createStructDecl(struct Node *root, int name, int body);
int createAnnotation(struct Node *root, int name, int type, int value);
int createModule(struct Node *root, enum NodeType type, int value);

/**
 * @brief Membuat request baru untuk parser.
 *
 * @param tokens Pointer ke struktur token.
 * @param capacity Kapasitas awal node.
 * @return Struktur Request yang terinisialisasi.
 */
struct Request createRequest(struct Token *tokens, int capacity);

/**
 * @brief Menghapus seluruh node dan mengosongkan buffer.
 *
 * @param node pointer node
 */
void clearNode(struct Node *node);

#endif
