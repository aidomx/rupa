#pragma once

#if defined(RUPA_PACKAGE_H)

extern void *process(State *state);
extern int processConstruct(State *state, int start, int end, bool *waiting);
extern int processKeyword(State *state, KeywordType type, int start, int next,
                          int end, bool *waiting);
extern bool scanKeyword(const char *source, int start, int end,
                        KeywordType *type, int *next);

/* construct handlers */
extern int processIdentifier(State *state, int start, int end, bool literal,
                             int *next);
extern int processString(State *state, int start, int end, int *next,
                         bool *waiting);
extern int processNumber(State *state, int start, int end, int *next,
                         bool *waiting);
extern int processOperator(State *state, int start, int end, int *next,
                           bool *waiting);
extern int processDelimiter(State *state, int start, int end, int *next,
                            int *brace, int *bracket, int *paren,
                            bool *expectValue);
#endif
