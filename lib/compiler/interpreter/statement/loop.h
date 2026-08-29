#pragma once
#if defined(RUPA_PACKAGE_H)

const char *identName(Node *node, int id);
bool isRangeLoop(const AstNode *ast);
bool rangeOperator(const char *kind, const char *op);

#endif
