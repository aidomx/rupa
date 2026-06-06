#include <rupa.h>

/**
 * @brief Memproses daftar identifier (x, y, z).
 *
 * Fungsi ini bersifat rekursif dan berhenti ketika
 * identifier list berakhir atau masuk mode waiting.
 *
 * @param atom Pointer ke Atom
 * @param flags Lexer flags
 * @param lex LexerState
 * @return Posisi cursor baru
 */
int resolveId(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  int end = lex->end;
  char current = content[end];
  // setelah , tidak ada karakter lain selain terminate
  if (check_terminate(current))
    return await(EXCEPT_IDENTIFIER, flags, lex->cursor, true);

  atom->cursor = end;
  lex->start = end;
  int result = lexeme(atom, content, EXCEPT_NONE);
  lex->end = atom->cursor;

  if (result == -1) {
    flags->isComplete = saveToken(lex) != -1;
    return lex->cursor;
  }

  if (!atom->has_next) {
    flags->isComplete = saveToken(lex) != -1;
    return lex->cursor;
  }

  if (saveToken(lex) == -1)
    return lex->cursor;

  addDelim(lex->token, atom->next, lex->at, lex->line, lex->row);
  flags->isComplete = true;

  lex->end++;
  return resolveId(atom, flags, lex);
}
