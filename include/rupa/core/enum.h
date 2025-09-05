/**
 * Superset tipe
 *
 * Mendefinisikan tipe sesuai dengan kategori :
 * - BinaryType
 * - ErrorType
 * - NodeType
 * - TokenType
 * - VariableType
 * - etc
 *
 * @author aidomx
 */
#pragma once

/**
 * Definisi tipe pada saat proses interpreter berjalan,
 * untuk daftar ErrorType dibantu oleh ChatGPT.
 *
 * @ErrorType
 * @reference ChatGPT
 */
enum ErrorType {
  ERR,  // Kesalahan umum / tidak diketahui
  WARN, // Peringatan umum

  // Tokenizer / Lexer errors
  ERR_UNEXPECTED_CHAR,  // Karakter tidak dikenali
  ERR_UNTERMINATED_STR, // String tidak ditutup
  ERR_INVALID_NUMBER,   // Format angka tidak valid

  // Parser errors
  ERR_SYNTAX,           // Kesalahan sintaks umum
  ERR_MISSING_TOKEN,    // Token penting hilang (mis. '}' atau ')')
  ERR_UNEXPECTED_TOKEN, // Token tidak sesuai konteks
  ERR_UNEXPECTED_EOF,   // Akhir file tidak terduga saat parsing

  // Semantic errors
  ERR_UNDEFINED_VAR,  // Variabel belum dideklarasikan
  ERR_REDECLARED_VAR, // Variabel sudah dideklarasikan
  ERR_TYPE_MISMATCH,  // Tipe tidak sesuai (mis. string ditambah int)
  ERR_INVALID_ASSIGN, // Penugasan ke konstanta atau rvalue

  // Runtime errors
  ERR_DIV_BY_ZERO,         // Pembagian oleh nol
  ERR_NULL_DEREF,          // Mengakses nilai null
  ERR_INDEX_OUT_OF_BOUNDS, // Akses indeks di luar batas
  ERR_INVALID_CALL,        // Pemanggilan fungsi pada non-fungsi
  ERR_STACK_OVERFLOW,      // Tumpukan melebihi batas

  // Warning types
  WARN_UNUSED_VAR,    // Variabel tidak digunakan
  WARN_DEPRECATED,    // Penggunaan fitur usang
  WARN_SHADOWING,     // Variabel menimpa variabel di scope luar
  WARN_POSSIBLE_NULL, // Potensi akses null

  // Internal/engine errors
  ERR_INTERNAL,       // Kesalahan internal interpreter
  ERR_NOT_IMPLEMENTED // Fitur belum diimplementasi
};

/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing menjadi AST.
 *
 * @NodeType
 */
enum NodeType {
  NODE_UNKNOWN = -1,
  NODE_ASSIGN = 0,
  NODE_BOOLEAN = 1,
  NODE_BINARY = 2,
  NODE_ENDPROGRAM = 3,
  NODE_FLOAT = 4,
  NODE_FUNCTION = 5,
  NODE_IDENTIFIER = 6,
  NODE_LITERAL_ID = 7,
  NODE_NUMBER = 8,
  NODE_NULLABLE = 9,
  NODE_PROGRAM = 10,
  NODE_PARAM = 11,
  NODE_RETURN = 12,
  NODE_STRING = 13,
  NODE_SUBSCRIPT = 14,
  NODE_VARIABLE = 15
};

/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing expression menjadi AST.
 *
 * @BinaryType
 */
enum BinaryType {
  BINARY_ADD = 0,
  BINARY_ASSIGN = 1,
  BINARY_MULTIPLY = 2,
  BINARY_SUBTRACT = 3,
  BINARY_SUBSCRIPT = 4,
  BINARY_DIVIDE = 5,
  BINARY_MODULES = 6,
  BINARY_NONE = -1,
};

/**
 * Definisi tipe yang digunakan untuk membagi jenis
 * tipe token saat proses tokenize atau lexer, dan
 * untuk daftar TokenType dibantu oleh ChatGPT.
 *
 * @TokenType
 * @reference ChatGPT
 */
enum TokenType {
  AMPERSAND = 0, // &
  ARROW = 1,     // ->
  ASSIGN = 2,    // =
  ASTERISK = 3,  // *
  AT = 4,        // @
  BACKSLASH = 5, // \backlash
  BACKSPACE = 6, // \b
  BACKTICK = 7,  // `
  BIGINT = 8,
  BITWISE = 9,
  BOOLEAN = 10,
  CARET = 11,           // ^
  CARRIAGE_RETURN = 12, // \r
  COLON = 13,           // :
  COMMA = 14,           // ,
  DECREMENT = 15,       // --
  DIVIDE = 16,          // /
  DOLLAR = 17,          // $
  DOT = 18,             // .
  DOUBLE = 19,
  ELLIPSIS = 20, // ...
  ENDOF = '\0',
  EQUAL = 21,       // ==
  EQUAL_THAN = 22,  // =
  EXCLAMATION = 23, // !
  FAT_ARROW = 24,   // =>
  FLOAT = 25,
  FORM_FEED = 26,     // \f
  GREATER_EQUAL = 27, // >=
  GREATER_THAN = 28,  // >
  HASHTAG = 29,       // #
  IDENTIFIER = 30,
  INCREMENT = 31, // ++
  INT = 32,
  KEYWORD = 33,
  LESS_EQUAL = 34, // <=
  LESS_THAN = 35,  // <
  LITERAL_ID = 36,
  LOGICAL_AND = 37, // &&
  LOGICAL_OR = 38,  // ||
  LBLOCK = 39,      // [
  RBLOCK = 40,      // ]
  LBRACE = 41,      // {
  RBRACE = 42,      // }
  LPAREN = 43,      // (
  RPAREN = 44,      // )
  MINUS = 45,       // -
  NEWLINE = 46,     // \n
  NOT_EQUAL = 47,   // !=
  NULLABLE = 48,    // NULL
  NUMBER = 49,
  PERCENT = 50,       // %
  PIPE = 51,          // |
  PLUS = 52,          // +
  QUESTION_MARK = 53, // ?
  QUOTE = 54,         // "
  SEMICOLON = 55,     // ;
  SHIFT_LEFT = 56,    // <<
  SHIFT_RIGHT = 57,   // >>
  SINGLE_QUOTE = 58,  // '
  SLASH = 59,         // /
  STAR = 60,          // *
  STRING = 61,
  TAB = 62,   // \t
  TILDE = 63, // ~
  UNKNOWN = -1
};

/**
 * Definisi tipe yang digunakan untuk menentukan
 * tipe dari variabel saat proses membangun AST.
 *
 * @VariableType
 */
enum VariableType {
  VAR_BIGINT,
  VAR_BOOLEAN,
  VAR_DOUBLE,
  VAR_FLOAT,
  VAR_INT,
  VAR_NUMBER,
  VAR_STRING,
};
