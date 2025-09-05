#pragma once

#include "rupa/package.h"

extern Response generateHandler(Request *req, Token *t, int init);
extern Response processAssignment(Request *req, Token *t, int init);
extern Response processStandaloneExpression(Request *req, Token *t, int init);
extern void handleUnexpectedToken(Request *req, DataToken *data, int init);
