#pragma once

#include "print_ast_shared.h"

#if defined(RUPA_PACKAGE_H)

extern void printAst(Node *node, int index, int level);
extern bool printBasicAst(Node *node, int index, int level);
extern bool printStructuralAst(Node *node, int index, int level);
extern bool printControlAst(Node *node, int index, int level);

#endif
