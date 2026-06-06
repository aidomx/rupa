#include <rupa.h>

/**
 * @brief Inisialisasi atom awal dari input.
 *
 * Atom merepresentasikan unit terkecil yang dapat di-lex
 * (identifier, number, underscore, dll).
 *
 * @param input Pointer ke Input
 * @return Atom terinisialisasi atau zeroed Atom jika gagal
 */
Atom init_atom(Input *input) {
  if (!input || input->length == 0)
    return (Atom){0};

  return (Atom){.content = NULL,
                .cursor = input->cursor,
                .depth = 0,
                .depth_brace = 0,
                .depth_bracket = 0,
                .depth_paren = 0,
                .has_next = false,
                .next = '\0',
                .number = false,
                .prev = '\0',
                .quote = false,
                .string = false,
                .underscore = false};
}

/**
 * @brief Menentukan atom utama (primary atom) pada posisi cursor.
 *
 * Fungsi ini hanya bertugas mengaktifkan flags->isIdentifier
 * tanpa memindahkan alur eksekusi lexer.
 *
 * @param atom Pointer ke Atom
 * @param content Source input
 * @param flags Lexer flags
 */
void primaryAtom(Atom *atom, const char *content, Flags *flags) {
  if (!atom || !content || !flags || atom->cursor == -1)
    return;

  if (!flags->isIdentifier) {
    flags->isIdentifier = lexeme(atom, content, EXCEPT_NONE) != -1;
  }
}

int scanAtom(Atom *atom) {
  if (!atom || !atom->content || atom->cursor == -1)
    return -1;

  const char *content = atom->content;

  int start = atom->cursor;
  skipWhitespace(content, &atom->cursor);
  char first = content[atom->cursor];

  int current_pos = isalpha(first) && scanId(atom);

  if (current_pos == -1)
    return -1;

  skipWhitespace(content, &atom->cursor);
  int end = atom->cursor;

  bool is_number = (atom->number && !atom->string && !atom->underscore);

  // after identifier
  // ex: x(, x[, x{, x:, x=, and [x[operator]]
  if (!nextChar(atom->next))
    return !is_number ? atom->cursor : -1;

  char word[512];
  int length = 0;
  for (int i = start; i < end; i++) {
    word[length++] = content[i];
  }

  if (length == 0)
    return -1;

  word[length] = '\0';
  bool is_boolean = (strcmp(word, "true") == 0 || strcmp(word, "false") == 0);

  return !is_boolean ? atom->cursor : -1;
}
