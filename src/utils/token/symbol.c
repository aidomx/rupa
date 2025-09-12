#include <rupa/package.h>

SymbolToken symbol_table[] = {{'+', PLUS},
                              {'-', MINUS},
                              {'*', ASTERISK},
                              {'/', SLASH},
                              {'%', PERCENT},
                              {'&', AMPERSAND},
                              {'|', PIPE},
                              {'^', CARET},
                              {'~', TILDE},
                              {'?', QUESTION_MARK},
                              {':', COLON},
                              {'.', DOT},
                              {',', COMMA},
                              {';', SEMICOLON},
                              {'@', AT},
                              {'$', DOLLAR},
                              {'!', EXCLAMATION},
                              {'<', LESS_THAN},
                              {'>', GREATER_THAN},
                              {'=', EQUAL_THAN},
                              {'#', HASHTAG},
                              {'\\', BACKSLASH},
                              {'`', BACKTICK},
                              {'"', QUOTE},
                              {'\'', SINGLE_QUOTE},
                              {'[', LBLOCK},
                              {']', RBLOCK},
                              {'{', LBRACE},
                              {'}', RBRACE},
                              {'(', LPAREN},
                              {')', RPAREN},
                              {'\n', NEWLINE},
                              {'\t', TAB},
                              {'\r', CARRIAGE_RETURN},
                              {'\b', BACKSPACE},
                              {'\f', FORM_FEED}};

const size_t SYMBOL_TABLE_SIZE = sizeof(symbol_table) / sizeof(symbol_table[0]);

SymbolToken *getSymbolToken(char c) {
  for (size_t i = 0; i < SYMBOL_TABLE_SIZE; i++) {
    if (symbol_table[i].symbol == c) {
      return &symbol_table[i];
    }
  }

  return NULL;
}
