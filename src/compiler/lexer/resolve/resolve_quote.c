#include <rupa.h>

int resolveQuote(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  if (isquote(atom->next))
    return lex->cursor;

  return resolveQuote(atom, flags, lex);
}
