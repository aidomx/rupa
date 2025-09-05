#pragma once
//#ifndef RUPA_UTILS_ASSIGNMENT_H
//#define RUPA_UTILS_ASSIGNMENT_H
#include "rupa/package.h"

extern bool isPartOfAssignment(Token *t, int pos);
extern bool isAssignmentStatement(Token *t, int init);
extern bool isAssignmentToken(Token *t, int init);
extern int findAssignmentOperator(Token *t, int start);
extern int getLeftSide(Token *t, int init);

// #endif // RUPA_UTILS_ASSIGNMENT_H
