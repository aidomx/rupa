#include <rupa.h>

void depth_open(Atom *atom, LexerState *lex) {
  if (!atom || check_lexer(lex))
    return;

  const char *content = lex->content;

  switch (atom->next) {
  case '[':
    atom->depth_bracket++;
    break;

  case '(':
    atom->depth_paren++;
    break;
  }

  lex->end++;
  save_delim(atom->next, lex);
  atom->next = content[lex->end];
}

void depth_close(Atom *atom, LexerState *lex) {
  if (!atom || check_lexer(lex))
    return;

  const char *content = lex->content;

  switch (atom->next) {
  case ']':
    atom->depth_bracket--;
    break;

  case ')':
    atom->depth_paren--;
    break;
  }

  lex->end++;
  save_delim(atom->next, lex);
  atom->next = content[lex->end];
}

int resolveBinary(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  switch (atom->next) {
  case '[':
  case '(':
    depth_open(atom, lex);

    if (atom->next == ']') {
      atom->depth_bracket--;
      save_delim(atom->next, lex);

      if (atom->depth_bracket == 0)
        return lex->cursor;

      lex->end++;
      return resolveBinary(atom, flags, lex);
    }

    else if (atom->next == ')') {
      atom->depth_paren--;
      save_delim(atom->next, lex);

      if (atom->depth_paren == 0)
        return lex->cursor;

      lex->end++;
      return resolveBinary(atom, flags, lex);
    }

    if (!check_terminate(atom->next))
      return resolveBinary(atom, flags, lex);

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
  return resolveBinary(atom, flags, lex);
}
