#include <rupa.h>

/**
 * @brief Menentukan langkah lexer selanjutnya setelah primary atom.
 *
 * Menangani:
 * - identifier tunggal
 * - identifier list
 * - except state
 *
 * @param atom Pointer ke Atom
 * @param flags Lexer flags
 * @param lex LexerState
 * @return Posisi cursor baru
 */
int resolveNextProgram(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  int end = lex->end;

  // ex: x
  if (!atom->has_next && !flags->isWaiting) {
    flags->isComplete = saveToken(lex) != -1;
    return lex->cursor;
  }

  end = save_lhs(atom, lex);
  // reset temporary
  if (atom->number || atom->string || atom->underscore || end != -1) {
    atom->number = false;
    atom->string = false;
    atom->underscore = false;
  }

  switch (atom->next) {
  case '=':
    printf("assignment\n");
    break;

  case ',':
    lex->end = end;
    end = resolveId(atom, flags, lex);
    break;

  default:
    return resolveExcept(atom, flags, lex);
  }

  return lex->cursor;
}

/**
 * @brief Entry point lexer untuk satu unit program.
 *
 * Fungsi ini:
 * - menginisialisasi atom dan lexer state
 * - menentukan primary atom
 * - menyelesaikan token atau masuk mode waiting
 *
 * @param state Pointer ke State
 * @return state atau NULL jika invalid
 */
void *resolveProgram(State *state) {
  if (check_state(state))
    return NULL;

  Editor *editor = state->repl->editor;
  Input *input = state->input;
  Flags *flags = input->flags;
  Token *token = state->tokens;

  Atom atom = init_atom(input);
  LexerState lex = init_lex(editor, input, token);

  const char *content = input->content;

  atom.prev = atom.next;
  lex.start = atom.cursor;
  primaryAtom(&atom, content, input->flags);
  lex.end = atom.cursor;
  int pos = atom.cursor;
  lex.cursor = consumeToEnd(content, pos);

  // is not identifier
  if (!flags->isIdentifier && !flags->isWaiting) {
    atom.next = content[pos];
    flags->isComplete = saveToken(&lex) != -1;
    // for symbol only
    // ex: ]]
    input->cursor = resolveSymbol(&atom, flags, &lex);
    return state;
  }

  input->cursor = resolveNextProgram(&atom, flags, &lex);
  flags->isComplete = !flags->isWaiting;
  return state;
}
