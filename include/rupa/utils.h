#ifndef UTILS_H
#define UTILS_H

#include "enum.h"

void clearScreen();
char getBracketType(const char *ptr);
TokenType gettype(const char *ptr);
// void replace(const char *str, const char *key, const char *value);
char *serialize(const char *str, char buffer[]);
char *substring(const char *input, int start, int end);
void trimquote(char *value);
void trimbracket(char *value, char open, char close);
char *trimspace(char *value);

#endif
