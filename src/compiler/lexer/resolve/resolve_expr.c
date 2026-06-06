#include <rupa.h>

static int resolveGrouped(Atom *atom, Flags *flags, LexerState *lex);

ExceptType getexcept(Atom *atom) {
  if (!atom)
    return EXCEPT_NONE;

  if (atom->prev == '(' && check_terminate(atom->next)) {
    atom->depth_paren++;
    return EXCEPT_EXPRESSION;
  }

  else if (atom->prev == '[' && check_terminate(atom->next)) {
    atom->depth_bracket++;
    return EXCEPT_ARRAY;
  }

  return EXCEPT_NONE;
}

static int resolveAtom(Atom *atom, Flags *flags, LexerState *lex) {
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
    return resolveExpression(atom, flags, lex);
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

int resolveExpression(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  ExceptType except = getexcept(atom);

  if (except != EXCEPT_NONE) {
    save_delim(atom->prev, lex);
    return await(except, flags, lex->cursor, true);
  }

  switch (atom->next) {
  case '[':
    return resolveArray(atom, flags, lex);

  case '(':
    break;

  default:
    return save_rhs(atom, flags, lex);
  }

  return lex->cursor;
}
