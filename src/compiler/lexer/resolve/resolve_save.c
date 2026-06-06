#include <rupa.h>

void save_delim(char c, LexerState *lex) {
  if (check_lexer(lex) || !issymbol(c))
    return;

  printf("delim: %c\n", c);

  addDelim(lex->token, c, lex->at, lex->line, lex->row);
}

int save_lhs(Atom *atom, LexerState *lex) {
  if (!atom || check_lexer(lex))
    return -1;

  if (saveToken(lex) == -1)
    return lex->cursor;

  save_delim(atom->next, lex);
  return lex->end;
}

int save_rhs(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  // re-lexeme
  relex(atom, flags->except, lex);

  if (check_terminate(atom->next)) {
    flags->isComplete = saveToken(lex) != -1 && !flags->isWaiting;
    return lex->cursor;
  }

  saveToken(lex);
  save_delim(atom->next, lex);
  flags->isComplete = !flags->isWaiting;
  return lex->end;
}
