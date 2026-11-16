#pragma once
#if defined(RUPA_PACKAGE_H)

extern struct Response processAssignment(struct Request *req, struct Token *t,
                                         int init);

/**
 * @brief Loop utama untuk membangun AST dari token list.
 *
 * @param req Pointer ke Request parser.
 * @return Pointer ke Node AST root.
 */
extern struct Node *processGenerate(struct Request *req);

#endif
