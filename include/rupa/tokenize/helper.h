#pragma once

#include "rupa/package.h"

extern Array getArrayIndex(const char *input, int start, int end);
extern char *getTokenId(const char *input, int start, int end);
extern char *getTokenValue(const char *input, int start, int end);
extern int handle_quotes(char c, int *in_quotes, int *quote_char);
extern int handle_braces(char c, int brace_level, State *state);
extern int handle_whitespace(const char *next_char, int in_quotes,
                             State *state);
extern int handle_regular_char(char c, State *state);
extern int should_add_newline(int i, int length, char **history, State *state);
