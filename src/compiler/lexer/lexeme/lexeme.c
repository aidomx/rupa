#include <rupa.h>

static void debug_atom(Atom *atom, int show) {
  if (!show)
    return;

  printf("prev: %c\nnext: %c\nnumber: %d\nstring: %d\nunderscore: %d\nhas "
         "next: %d\n",
         atom->prev, atom->next, atom->number, atom->string, atom->underscore,
         atom->has_next);
}
/**
 * @brief Daftar literal atom yang tidak valid sebagai identifier.
 *
 * Digunakan untuk menyaring lexeme agar tidak dianggap identifier,
 * meskipun secara bentuk memenuhi aturan (huruf/angka/underscore).
 *
 * Catatan:
 * - Keyword bahasa TIDAK didefinisikan di sini.
 * - Array ini hanya berisi literal, alias nilai, dan simbol semantik.
 */
static const char *literal_atoms[] = {
    // boolean literal
    "true", "TRUE", "false", "FALSE",

    // internal / magic
    "__proto__", "__self__", "__this__", "__global__", "__local__",

    // nullish
    "null", "NULL", "nil", "NIL", "none", "NONE",

    // numeric alias
    "0", "1", "00", "01", "0x", "0X", "0b", "0B",

    // pseudo type / value hint
    "number", "NUMBER", "string", "STRING", "bool", "BOOL", "boolean",
    "BOOLEAN", "object", "OBJECT", "array", "ARRAY", "map", "MAP",

    // special values
    "nan", "NaN", "inf", "INF", "infinity", "INFINITY",

    // symbol words
    "and", "or", "not", "xor", "and", "or", "not", "xor"};
static size_t la_size = sizeof(literal_atoms) / sizeof(literal_atoms[0]);

/**
 * @brief Mengecek apakah sebuah potongan lexeme termasuk literal atom.
 *
 * @param content Pointer ke awal lexeme
 * @param length Panjang lexeme
 * @return true jika termasuk literal atom (invalid identifier)
 * @return false jika aman sebagai identifier
 *
 * Fungsi ini bersifat *pure check*, tidak memodifikasi state apa pun.
 */
static bool la_status(const char *content, int length) {
  if (!content || length == -1)
    return false;

  bool match = false;
  for (size_t i = 0; i < la_size; i++) {
    int atom_len = strlen(literal_atoms[i]);

    if (length == atom_len && strncmp(content, literal_atoms[i], length) == 0) {
      match = true;
      break;
    }
  }

  return match;
}

static bool is_number(Atom *atom) {
  return atom->number && !atom->string && !atom->underscore;
}

static bool is_except(ExceptType except) {
  switch (except) {
  case EXCEPT_ASSIGNMENT:
  case EXCEPT_NONE:
    return true;

  default:
    return false;
  }
}

/**
 * @brief Membangun lexeme utama dan menentukan validitas identifier.
 *
 * Fungsi ini adalah entry utama scanner untuk:
 * - identifier
 * - numeric (int / float / double)
 * - underscore-based symbol
 *
 * @param atom Struktur atom untuk menyimpan hasil scanning
 * @param content Source input
 * @return Posisi cursor akhir jika valid identifier, -1 jika tidak valid
 *
 * Catatan desain:
 * - Lexeme bersifat tegas: return -1 ≠ error, tapi "bukan identifier".
 * - Literal atom disaring terakhir menggunakan la_status().
 * - Fungsi ini boleh dipanggil berulang untuk membangun struktur lebih
 * kompleks.
 */
int lexeme(Atom *atom, const char *content, ExceptType except) {
  if (!atom || !content || atom->cursor == -1)
    return -1;

  int response = -1;
  int start = atom->cursor;
  skipWhitespace(content, &atom->cursor);
  char first = content[atom->cursor];
  atom->prev = first;

  // first character is string
  if (isalpha(first))
    response = scanString(atom, content);

  // first character is number
  else if (isdigit(first))
    response = scanNumber(atom, content);

  // first character is underscore
  else if (isunderscore(first))
    response = scanUnderscore(atom, content);

  // mencegah kebocoran karakter
  if (response <= -1)
    return -1;

  int end = atom->cursor;

  debug_atom(atom, 0);

  // invalid identifier
  // 1.0 is'nt split but split for 0,1
  // split in array, grouped
  if (is_number(atom) && !is_except(except) && isdot(atom->next)) {
    if (!atom->has_next)
      return -1;

    atom->cursor++;
    return lexeme(atom, content, except);
  }

  // 0,1 or 1.0 is'nt slice
  if (is_number(atom) && is_except(except)) {
    if (!atom->has_next)
      return -1;

    atom->cursor++;
    return lexeme(atom, content, except);
  }

  atom->content = content + start;
  int length = end - start;

  return !la_status(atom->content, length - 1) ? atom->cursor : -1;
}
