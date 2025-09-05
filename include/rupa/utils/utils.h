#pragma once

#include "rupa/package.h"

extern void clearScreen();
extern int findArr(Token *tokens, int pos);
extern char getBracketType(const char *ptr);
extern TokenType gettype(const char *ptr);
// void replace(const char *str, const char *key, const char *value);
extern char *serialize(const char *str, char buffer[]);
extern char *substring(const char *input, int start, int end);
extern void trimquote(char *value);
extern void trimbracket(char *value, char open, char close);
extern char *trimspace(char *value);
