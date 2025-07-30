/**
 * Superset tipe
 *
 * Mendefinisikan tipe sesuai dengan kategori :
 * - ErrorType
 * - NodeType
 * - TokenType
 *
 * @author aidomx
 */
#ifndef ENUM_H
#define ENUM_H

typedef enum {
  AST_ASSIGN,
  AST_BINARY,
  AST_BRACKET,
  AST_BOOLEAN,
  AST_CALL,
  AST_DELIM,
  AST_IDENTIFIER,
  AST_FLOAT,
  AST_NUMBER,
  AST_STRING
} AstType;

/**
 * Definisi tipe pada saat proses interpreter berjalan,
 * untuk daftar ErrorType dibantu oleh ChatGPT.
 *
 * @ErrorType
 * @reference ChatGPT
 */
typedef enum {
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
} ErrorType;

/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing menjadi AST.
 *
 * @NodeType
 */
typedef enum {
  NODE_ASSIGN,
  NODE_BINARY,
  NODE_FUNCTION,
  NODE_IDENTIFIER,
  NODE_NUMBER,
  NODE_PARAM,
  NODE_PROGRAM,
  NODE_RETURN,
  NODE_STRING,
  NODE_VARIABLE
} NodeType;

/**
 * Definisi tipe yang digunakan untuk menentukan tipe
 * saat proses parsing expression menjadi AST.
 *
 * @BinaryType
 */
typedef enum {
  BINARY_ADD,
  BINARY_MULTIPLY,
  BINARY_SUBTRACT,
  BINARY_DIVIDE,
  BINARY_MODULES
} BinaryType;

/**
 * Definisi tipe yang digunakan untuk membagi jenis
 * tipe token saat proses tokenize atau lexer, dan
 * untuk daftar TokenType dibantu oleh ChatGPT.
 *
 * @TokenType
 * @reference ChatGPT
 */
typedef enum {
  AMPERSAND, // &
  ARROW,     // ->
  ASSIGN,    // =
  ASTERISK,  // *
  AT,        // @
  BACKSLASH, // \backlash
  BACKSPACE, // \b
  BACKTICK,  // `
  BIGINT,
  BITWISE,
  BLOCK_LEFT,  // [
  BLOCK_RIGHT, // ]
  BOOLEAN,
  BRACE_LEFT,      // {
  BRACE_RIGHT,     // }
  CARET,           // ^
  CARRIAGE_RETURN, // \r
  COLON,           // :
  COMMA,           // ,
  DECREMENT,       // --
  DIVIDE,          // /
  DOLLAR,          // $
  DOT,             // .
  DOUBLE,
  ELLIPSIS, // ...
  ENDOF,
  EQUAL,       // ==
  EQUAL_THAN,  // =
  EXCLAMATION, // !
  FAT_ARROW,   // =>
  FLOAT,
  FORM_FEED,     // \f
  GREATER_EQUAL, // >=
  GREATER_THAN,  // >
  HASHTAG,       // #
  IDENTIFIER,
  INCREMENT, // ++
  INT,
  KEYWORD,
  LESS_EQUAL, // <=
  LESS_THAN,  // <
  LITERAL_ID,
  LOGICAL_AND, // &&
  LOGICAL_OR,  // ||
  MINUS,       // -
  NEWLINE,     // \n
  NOT_EQUAL,   // !=
  NUMBER,
  PAREN_LEFT,    // (
  PAREN_RIGHT,   // )
  PERCENT,       // %
  PIPE,          // |
  PLUS,          // +
  QUESTION_MARK, // ?
  QUOTE,         // "
  SEMICOLON,     // ;
  SHIFT_LEFT,    // <<
  SHIFT_RIGHT,   // >>
  SINGLE_QUOTE,  // '
  SLASH,         // /
  STRING,
  TAB,   // \t
  TILDE, // ~
  UNKNOWN
} TokenType;

/**
 * Definisi tipe yang digunakan untuk menentukan
 * tipe dari variabel saat proses membangun AST.
 *
 * @VariableType
 */
typedef enum {
  VAR_BIGINT,
  VAR_BOOLEAN,
  VAR_DOUBLE,
  VAR_FLOAT,
  VAR_INT,
  VAR_NUMBER,
  VAR_STRING,
} VariableType;

#endif
