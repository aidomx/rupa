#pragma once

#include "rupa/package.h"

/**
 * @brief Melakukan tokenisasi terhadap input tertentu.
 *
 * @param state ReplState
 */
extern Token *tokenize(Token *tokens, char **history, int length, int line);
