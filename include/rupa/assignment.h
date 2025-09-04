#ifndef RUPA_UTILS_ASSIGNMENT_H
#define RUPA_UTILS_ASSIGNMENT_H

#include "rupa/structure.h"
#include <stdbool.h>

bool isPartOfAssignment(Token *t, int pos);
bool isAssignmentStatement(Token *t, int init);
bool isAssignmentToken(Token *t, int init);
int findAssignmentOperator(Token *t, int start);
int getLeftSide(Token *t, int init);

#endif // RUPA_UTILS_ASSIGNMENT_H
