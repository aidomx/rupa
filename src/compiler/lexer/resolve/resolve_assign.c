#include <rupa.h>

static int resolveGrouped(Atom *atom, Flags *flags, LexerState *lex);

int resolveExpr(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  skipWhitespace(lex->content, &lex->end);
  int end = save_rhs(atom, flags, lex);
  if (end >= lex->cursor)
    return lex->cursor;
  atom->next = lex->content[end + 1];

  switch (atom->next) {
  case '(':
    lex->end++;
    return resolveGrouped(atom, flags, lex);

  case '[':
    lex->end++;
    return resolveArray(atom, flags, lex);
  }

  lex->end++;
  return resolveExpr(atom, flags, lex);
}

int resolveAtom(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;
  skipWhitespace(content, &lex->end);
  int end = save_rhs(atom, flags, lex);

  // for handle atom
  // ex: true, name, 1
  if (end >= lex->cursor)
    return lex->cursor;

  /*debug_pos("atom", lex->end, lex->cursor);*/
  /*debug_char(atom->prev, atom->next);*/

  switch (atom->next) {
  case '.':
    lex->end++;
    return resolveAtom(atom, flags, lex);

  case '[':
    atom->depth_bracket++;
    lex->end++;
    resolveArray(atom, flags, lex);
    lex->end++;
    atom->next = content[lex->end];

    return !check_terminate(atom->next) ? resolveAtom(atom, flags, lex)
                                        : lex->cursor;

  case '(':
    atom->depth++;
    lex->end++;
    resolveGrouped(atom, flags, lex);
    lex->end++;
    atom->next = content[lex->end];

    return !check_terminate(atom->next) ? resolveAtom(atom, flags, lex)
                                        : lex->cursor;

  default:
    lex->end++;
    return resolveExpr(atom, flags, lex);
  }

  return lex->cursor;
}

int resolveGrouped(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  switch (atom->next) {
  case '(':
    atom->depth++;
    lex->end++;
    save_delim(atom->next, lex);

    if (content[lex->end] == ')') {
      atom->depth--;
      save_delim(content[lex->end], lex);

      if (atom->depth == 0)
        return lex->cursor;

      lex->end++;
      return resolveGrouped(atom, flags, lex);
    }

    if (!check_terminate(content[lex->end]))
      return resolveGrouped(atom, flags, lex);

    return await(EXCEPT_ASSIGNMENT, flags, lex->cursor, true);

  case ')':
    atom->depth--;
    save_delim(atom->next, lex);
    return (atom->depth == 0) ? lex->end : lex->cursor;

  default:
    save_rhs(atom, flags, lex);
    break;
  }

  // stop recursive
  if (lex->cursor <= lex->end)
    return lex->cursor;

  lex->end++;
  return resolveGrouped(atom, flags, lex);
}

int resolveAssign(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  // setelah = tidak ada karakter lain selain terminate
  if (isassign(atom->prev) && check_terminate(atom->next))
    return await(EXCEPT_ASSIGNMENT, flags, lex->cursor, true);

  switch (atom->next) {
  case '(':
    resolveGrouped(atom, flags, lex);
    lex->end++;
    atom->next = content[lex->end];
    return !check_terminate(atom->next) ? resolveAtom(atom, flags, lex)
                                        : lex->cursor;

  case '[':
    resolveArray(atom, flags, lex);
    lex->end++;
    atom->next = content[lex->end];
    return !check_terminate(atom->next) ? resolveAtom(atom, flags, lex)
                                        : lex->cursor;

  case '"':
    return resolveQuote(atom, flags, lex);

  default:
    return resolveAtom(atom, flags, lex);
  }

  return lex->cursor;
}
