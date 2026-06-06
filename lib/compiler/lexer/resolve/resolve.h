#pragma once

#define debug_char(prev, next) printf("prev: %c, next: %c\n", prev, next)
#define debug_pos(args, x, y) printf("(%s): x: %d, y: %d\n", args, x, y);

#include "resolve_array.h"
#include "resolve_assign.h"
#include "resolve_binary.h"
#include "resolve_except.h"
#include "resolve_expression.h"
#include "resolve_id.h"
#include "resolve_keyword.h"
#include "resolve_program.h"
#include "resolve_quote.h"
#include "resolve_save.h"
#include "resolve_symbol.h"

#if defined(RUPA_PACKAGE_H)

extern int await(ExceptType except, Flags *flags, int end_pos, bool prepend);
extern bool check_terminate(char c);
extern int relex(Atom *atom, ExceptType except, LexerState *lex);
extern void *resolve(State *state, const char *args);

#endif
