#include <rupa.h>

int resolveSymbol(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->next = content[lex->end];

  if (check_terminate(atom->next)) {
    flags->isComplete = true;
    return lex->cursor;
  }

  save_delim(atom->next, lex);
  lex->end++;
  return resolveSymbol(atom, flags, lex);
}
