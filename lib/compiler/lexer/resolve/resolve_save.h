#pragma once

#if defined(RUPA_PACKAGE_H)

extern void save_delim(char c, LexerState *lex);
extern int save_lhs(Atom *atom, LexerState *lex);
extern int save_rhs(Atom *atom, Flags *flags, LexerState *lex);

#endif
