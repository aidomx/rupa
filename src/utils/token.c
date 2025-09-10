#include <rupa/package.h>

int findArr(Token *tokens, int pos) {
  if (!tokens || pos < 0 || pos >= tokens->length)
    return -1;

  // Jika posisi saat ini adalah LBLOCK, cari RBLOCK yang sesuai
  if (match(&tokens->data[pos], LBLOCK)) {
    int depth = 1;

    for (int i = pos + 1; i < tokens->length; i++) {
      if (match(&tokens->data[i], LBLOCK)) {
        depth++;
      } else if (match(&tokens->data[i], RBLOCK)) {
        depth--;
        if (depth == 0) {
          return i; // Mengembalikan posisi RBLOCK yang sesuai
        }
      }
    }
    return -1; // Tidak ditemukan RBLOCK yang sesuai
  }
  // Jika posisi saat ini adalah RBLOCK, cari LBLOCK yang sesuai
  else if (match(&tokens->data[pos], RBLOCK)) {
    int depth = 1;

    for (int i = pos - 1; i >= 0; i--) {
      if (match(&tokens->data[i], RBLOCK)) {
        depth++;
      } else if (match(&tokens->data[i], LBLOCK)) {
        depth--;
        if (depth == 0) {
          return i; // Mengembalikan posisi LBLOCK yang sesuai
        }
      }
    }
    return -1; // Tidak ditemukan LBLOCK yang sesuai
  }

  return -1; // Bukan LBLOCK atau RBLOCK
}

int findComma(Token *t, int start, int end) {
  if (!t || start >= end)
    return -1;

  for (int i = start; i < end; i++) {
    if (isToken(t, i, COMMA)) {
      return i;
    }
  }

  return -1;
}

int findParen(Token *tokens, int start, int end) {
  if (!tokens || start >= end)
    return -1;

  int depth = 1;
  for (int i = start + 1; i < end; i++) {
    if (match(&tokens->data[i], LPAREN)) {
      depth++;
    } else if (match(&tokens->data[i], RPAREN)) {
      depth--;
      if (depth == 0)
        return i;
    }
  }
  return -1;
}

bool match(DataToken *data, TokenType T) { return data && data->type == T; }

bool isToken(Token *t, int pos, TokenType type) {
  return (pos >= 0 && pos < t->length && match(&t->data[pos], type));
}

DataToken *getToken(Token *t, int pos) {
  return (pos < 0 || pos >= t->length) ? NULL : &t->data[pos];
}

int lastIndex(Token *token, int start) {
  if (!token)
    return start;
  int currentLine = token->data[start].line;
  while (start < token->length && token->data[start].line == currentLine) {
    start++;
  }
  return start;
}

static const SymbolToken symbol_table[] = {{'+', PLUS},
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

static TokenType lookup_symbol(char c) {
  for (size_t i = 0; i < sizeof(symbol_table) / sizeof(symbol_table[0]); i++) {
    if (symbol_table[i].symbol == c)
      return symbol_table[i].type;
  }
  return UNKNOWN;
}

TokenType gettype(const char *ptr) {
  if (strcmp(ptr, "true") == 0 || strcmp(ptr, "false") == 0)
    return BOOLEAN;

  if (strcmp(ptr, "null") == 0) {
    return NULLABLE;
  }

  if (isquote(*ptr)) {
    char quote = *ptr;
    const char *end = ptr + 1;
    while (*end && *end != quote)
      end++;
    return (*end == quote) ? STRING : UNKNOWN;
  }

  if (isint(*ptr)) {
    const char *p = ptr;
    bool has_digit = false, has_dot = false;

    while (isint(*p)) {
      has_digit = true;
      p++;
    }

    if (isdot(*p)) {
      has_dot = true;
      p++;
      bool has_frac = false;
      while (isint(*p)) {
        has_frac = true;
        p++;
      }
      if (!has_frac)
        return UNKNOWN;
    }

    return (has_digit && !has_dot) ? NUMBER : FLOAT;
  }

  if (isstr(*ptr)) {
    const char *p = ptr + 1;
    while (*p && (isstr(*p) || isint(*p)))
      p++;
    return (*p == '\0') ? IDENTIFIER : UNKNOWN;
  }

  else {
    TokenType t = lookup_symbol(*ptr);
    if (t != UNKNOWN)
      return t;
  }

  return UNKNOWN;
}
