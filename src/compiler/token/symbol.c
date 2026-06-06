#include <rupa.h>

struct Symbol symbolList[] = {{'+', PLUS},
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
                              {'[', LBRACKET},
                              {']', RBRACKET},
                              {'{', LBRACE},
                              {'}', RBRACE},
                              {'(', LPAREN},
                              {')', RPAREN},
                              {'\n', NEWLINE},
                              {'\t', TAB},
                              {'\r', CARRIAGE_RETURN},
                              {'\b', BACKSPACE},
                              {'\f', FORM_FEED},
                              {'_', UNDERLINE}};

const size_t SYMBOL_LIST_SIZE = sizeof(symbolList) / sizeof(symbolList[0]);

Symbol *getSymbolToken(char c) {
  for (size_t i = 0; i < SYMBOL_LIST_SIZE; i++) {
    if (symbolList[i].token == c) {
      return &symbolList[i];
    }
  }
  return NULL;
}
