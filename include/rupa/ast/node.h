#pragma once

#include "rupa/package.h"

void addToProgram(Node *node, int programId, int declId);

/**
 * @brief Menghapus seluru node dan mengosongkan buffer.
 *
 * @param node pointer node
 */
void clearNode(Node *node);

int createArray(Node *root, int *elements, int length);

/**
 * @brief Menambahkan node AST ke dalam struktur pohon.
 *
 * @param node Node induk.
 * @param n Node AST baru.
 * @return Status keberhasilan.
 */
int createAst(Node *node, AstNode n);
int createBoolean(Node *root, bool value);
int createFloat(Node *root, char *value);

/**
 * @brief Membuat identifier node (variabel) dalam AST.
 *
 * @param root Root node AST.
 * @param name Nama identifier.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createId(Node *root, char *name, char *safetyType);

/**
 * @brief Membuat number literal node dalam AST.
 *
 * @param root Root node AST.
 * @param value Nilai integer.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createNumber(Node *root, int value);

int createProgram(Node *root);
int createReturn(Node *root, int expression_id);
int createString(Node *root, char *value, NodeType nodeType);

int createSubscript(Node *root, int posId, int index);

/**
 * @brief Membuat binary operation node dalam AST.
 *
 * @param root Root node AST.
 * @param opToken Operator (+, -, *, /, =, dsb).
 * @param leftId ID node operand kiri.
 * @param rightId ID node operand kanan.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createBinary(Node *root, DataToken *opToken, int leftId, int rightId);

/**
 * @brief Membuat assignment node dalam AST.
 *
 * @param root Root node AST.
 * @param left ID node target (identifier).
 * @param right ID node ekspresi nilai.
 * @return ID node yang dibuat, atau -1 jika gagal.
 */
int createAssignment(Node *root, int left, int right);

/**
 * @brief Membuat node AST baru.
 *
 * @param capacity Jumlah maksimum node anak.
 * @return Pointer ke Node yang baru.
 */
Node *createNode(int capacity);

/**
 * @brief Membuat request baru untuk parser.
 *
 * @param tokens Pointer ke struktur token.
 * @param capacity Kapasitas awal node.
 * @return Struktur Request yang terinisialisasi.
 */
Request createRequest(Token *tokens, int capacity);

int parseArray(Request *req, Response res, int start, int end);

/**
 * @brief Mem-parse sebuah atom (IDENTIFIER atau NUMBER).
 *
 * @param req Pointer ke Request parser.
 * @param data Pointer ke DataToken.
 * @return Index node hasil parse, atau -1 jika gagal.
 */
int parseAtom(Request *req, DataToken *data);

int parseBinary(Request *req, int start, int end);

/**
 * @brief Mem-parse faktor assignment (IDENTIFIER).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Index node identifier, atau -1 jika gagal.
 */
int parseFactor(Request *req, Response res);

int parseSubscripts(Request *req, int baseId, int start);

extern Response generateHandler(Request *req, Token *t, int init);
extern Response processAssignment(Request *req, Token *t, int init);
extern Response processStandaloneExpression(Request *req, Token *t, int init);
extern void handleUnexpectedToken(Request *req, DataToken *data, int init);

/**
 * @brief Mem-parse sebuah ekspresi assignment (kanan "=").
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
Response parseExpression(Request *req, Response res);

/**
 * @brief Mem-parse sebuah statement (assignment).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
Response parseStatement(Request *req, Response res);

/**
 * @brief Loop utama untuk membangun AST dari token list.
 *
 * @param req Pointer ke Request parser.
 * @return Pointer ke Node AST root.
 */
Node *processGenerate(Request *req);

/**
 * @brief Entry point parser: generate AST dari token list.
 *
 * @param tokens Pointer ke Token list.
 */
void generateAst(Token *tokens);

/**
 * @brief Proses debugging struktur AST.
 *
 * @param node Root node AST.
 */
void startDebug(Node *node);
