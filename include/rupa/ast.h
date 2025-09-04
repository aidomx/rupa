#ifndef RUPA_UTILS_AST_H
#define RUPA_UTILS_AST_H

#include "rupa/structure.h"

Response generateHandler(Request *req, Token *t, int init);
Response processAssignment(Request *req, Token *t, int init);
Response processStandaloneExpression(Request *req, Token *t, int init);
void handleUnexpectedToken(Request *req, DataToken *data, int init);

#endif
