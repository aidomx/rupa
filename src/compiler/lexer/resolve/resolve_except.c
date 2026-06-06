#include <rupa.h>

/**
 * @brief Menyelesaikan kondisi EXCEPT_IDENTIFIER.
 *
 * Digunakan saat lexer berada pada state menunggu identifier
 * (misalnya setelah koma atau newline).
 *
 * @param atom Pointer ke Atom
 * @param flags Lexer flags
 * @param lex LexerState
 * @return Posisi cursor baru
 */
int resolveExcept(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->prev = atom->next;
  int end = lex->end;
  atom->next = content[end];

  switch (flags->except) {
  case EXCEPT_IDENTIFIER:
    if (!flags->isWaiting)
      return lex->cursor;

    atom->cursor = end;
    lex->start = end;
    lexeme(atom, content, EXCEPT_IDENTIFIER);
    lex->end = atom->cursor;

    if (!atom->has_next && saveToken(lex) == -1)
      return lex->cursor;

    flags->isWaiting = false;
    end = lex->end;
    break;

  case EXCEPT_ARRAY:
    if (islbracket(atom->prev) && check_terminate(atom->next)) {
      atom->depth++;
      save_delim(atom->prev, lex);
      return lex->cursor;
    }

    if (isrbracket(atom->next)) {
      atom->depth--;
      save_delim(atom->next, lex);

      if (atom->depth == 0) {
        // array complete []
        flags->except = EXCEPT_NONE;
        flags->isWaiting = false;
        return lex->cursor;
      }

      lex->end++;
      return resolveExcept(atom, flags, lex);
    }

    if (lex->cursor <= lex->end)
      return lex->cursor;

    return save_rhs(atom, flags, lex);

  case EXCEPT_ASSIGNMENT:
    if (islbracket(atom->next)) {
      atom->depth++;
      save_delim(atom->next, lex);

      atom->next = content[end + 1];

      if (check_terminate(atom->next))
        return await(EXCEPT_ASSIGNMENT, flags, lex->cursor, true);
    }
    break;

  default:
    break;
  }

  return end;
}
