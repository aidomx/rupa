#pragma once
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
  DECIMAL = 19, // decimal literal; precision resolved by type context
  ELLIPSIS = 20, // ...
  ENDOF = '\0',
  EQUAL = 21,       // ==
  EQUAL_THAN = 22,  // =
  EXCLAMATION = 23, // !
  FAT_ARROW = 24,   // =>
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
  LBRACKET = 65,    // [
  RBRACKET = 66,    // ]
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
  CONDITIONAL_ASSIGN = 67, // ?=
  QUOTE = 54,         // "
  SEMICOLON = 55,     // ;
  SHIFT_LEFT = 56,    // <<
  SHIFT_RIGHT = 57,   // >>
  SINGLE_QUOTE = 58,  // '
  SLASH = 59,         // /
  STAR = 60,          // *
  STRING = 61,
  TAB = 62,       // \t
  TILDE = 63,     // ~
  UNDERLINE = 64, // _
  UNKNOWN = -1
};
