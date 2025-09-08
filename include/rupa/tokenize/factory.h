#pragma once

#include "rupa/package.h"

extern int createTokenId(State *state, int start, int end);

extern DataToken createDataToken(char *input, char *safetyType,
                                 TokenType tokenType, int line, int row);
