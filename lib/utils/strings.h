#pragma once

#if defined(RUPA_PACKAGE_H)

extern int getString(const char *buffer, int *pos);
extern int getUnderscore(const char *buffer, int *pos);
extern int removeChar(const char *str, char c, int pos);
// void replace(const char *str, const char *key, const char *value);
extern char *serialize(const char *str, char buffer[]);
extern char *substring(const char *input, int start, int end);
extern void trimquote(char *value);
extern void trimbracket(char *value, char open, char close);
extern char *trimspace(char *value);

#endif
