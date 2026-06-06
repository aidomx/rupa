#include <rupa.h>

int resolveArray(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  switch (atom->next) {
  case '[':
    atom->depth_bracket++;
    lex->end++;
    save_delim(atom->next, lex);

    if (content[lex->end] == ']') {
      atom->depth_bracket--;
      save_delim(content[lex->end], lex);

      if (atom->depth_bracket == 0)
        return lex->cursor;

      lex->end++;
      return resolveArray(atom, flags, lex);
    }

    if (!check_terminate(content[lex->end]))
      return resolveArray(atom, flags, lex);

    return await(EXCEPT_ARRAY, flags, lex->cursor, true);

  case ']':
    atom->depth_bracket--;
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
  return resolveArray(atom, flags, lex);
}
