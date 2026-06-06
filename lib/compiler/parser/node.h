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
 * @brief Menambahkan node AST ke dalam struktur pohon.
 *
 * @param node Node induk.
 * @param n Node AST baru.
 * @return Status keberhasilan.
 */
int createAst(struct Node *node, struct AstNode n);
int createBoolean(struct Node *root, bool value);
int createFloat(struct Node *root, char *value);

/**
 * @brief Membuat identifier node (variabel) dalam AST.
 *
 * @param root Root node AST.
 * @param name Nama identifier.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createId(struct Node *root, char *name, char *safetyType);

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
int createAssignment(struct Node *root, int left, int right);

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
