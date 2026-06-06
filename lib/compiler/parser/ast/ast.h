#pragma once
/**
 * @brief Paket Node
 *
 * Untuk membangun Abstract Syntax Tree
 */
#if defined(RUPA_PACKAGE_H)
/**
 * @brief Entry point parser: generate AST dari token list.
 *
 * @param tokens Pointer ke Token list.
 */
void generateAst(struct Token *tokens);

extern struct Response generateHandler(struct Request *req, struct Token *t,
                                       int init);

extern void handleUnexpectedToken(struct Request *req, struct DataToken *data,
                                  int init);

#endif
