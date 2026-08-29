#pragma once

#if defined(RUPA_PACKAGE_H)
extern bool isPartOfAssignment(struct Token *t, int pos);
extern bool isAssignmentStatement(struct Token *t, int init);
extern int findAssignmentOperator(struct Token *t, int start);
extern int getLeftSide(struct Token *t, int init);
#endif
