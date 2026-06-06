#pragma once

#include "ast/assignment.h"
#include "ast/ast.h"
#include "ast/expression.h"
#include "ast/operator.h"
#include "ast/processor.h"
#include "node.h"

#if defined(RUPA_PACKAGE_H)

int parseArray(struct Request *req, struct Response res, int start, int end);

/**
 * @brief Mem-parse sebuah atom (IDENTIFIER atau NUMBER).
 *
 * @param req Pointer ke Request parser.
 * @param data Pointer ke DataToken.
 * @return Index node hasil parse, atau -1 jika gagal.
 */
int parseAtom(struct Request *req, struct DataToken *data);

int parseBinary(struct Request *req, int start, int end);

/**
 * @brief Mem-parse faktor assignment (IDENTIFIER).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Index node identifier, atau -1 jika gagal.
 */
int parseFactor(struct Request *req, struct Response res);

int parseSubscripts(struct Request *req, int baseId, int start);

/**
 * @brief Mem-parse sebuah ekspresi assignment (kanan "=").
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
struct Response parseExpression(struct Request *req, struct Response res);

/**
 * @brief Mem-parse sebuah statement (assignment).
 *
 * @param req Pointer ke Request parser.
 * @param res Struktur Response sementara.
 * @return Struktur Response hasil parse.
 */
struct Response parseStatement(struct Request *req, struct Response res);

#endif
