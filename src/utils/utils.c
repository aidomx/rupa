#include "rupa/enum.h"
#include <ctype.h>
#include <rupa/package.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clearScreen() {
#ifdef _WIN32
  system("cls")
#else
  system("clear");
#endif
}

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

char getBracketType(const char *ptr) {
  if (!ptr)
    return 0;

  for (; *ptr; ptr++) {
    if (isfn(*ptr))
      return *ptr;
  }

  return 0;
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

void replace(const char *str, const char *key, const char *value,
             char output[]) {
  if (!str || !key || !value || !output)
    return;

  char *p = strstr(str, key);
  if (!p) {
    strncpy(output, str, MAX_MESSAGE_LENGTH - 1);
    output[MAX_MESSAGE_LENGTH - 1] = '\0';
    return;
  }

  int prefix_len = p - str;
  int remaining_len = MAX_MESSAGE_LENGTH - 1;

  snprintf(output, remaining_len, "%.*s%s%s", prefix_len, str, value,
           p + strlen(key));
}

char *serialize(const char *str, char buffer[]) {
  int newline = 0, out = 0;

  while (*str != '\0') {
    if (isnewline(*str)) {
      if (!newline) {
        buffer[out++] = '\x1F';
        newline = 1;
      }
    } else {
      buffer[out++] = *str;
      newline = 0;
    }
    str++;
  }
  buffer[out] = '\0';

  return buffer;
}

char *substring(const char *input, int start, int end) {
  if (!input || start < 0 || end <= start)
    return NULL;

  int len = end - start;
  char *result = malloc(len + 1);
  if (!result)
    return NULL;

  strncpy(result, input + start, len);
  result[len] = '\0';

  return result;
}

void trimbracket(char *value, char open, char close) {
  if (!value)
    return;

  if (open == '(') {
    size_t n = strlen(value);
    memmove(value, value + 1, n);
    value[n] = '\0';
  } else if (close == ')') {
    size_t n = strlen(value) - 1;
    memmove(value, value, n);
    value[n] = '\0';
  }
}

char *trimspace(char *value) {
  if (value == NULL || *value == '\0')
    return value;

  char *dest = value;
  char *str = value;
  int in_quote = 0;
  char quote_char = '\0';

  while (*str) {
    if (isquote(*str)) {
      if (!in_quote) {
        in_quote = 1;
        quote_char = *str;
      }

      else if (in_quote && *str == quote_char) {
        in_quote = 0;
      }

      *dest++ = *str++;
      continue;
    }

    if (in_quote) {
      *dest++ = *str++;
    }

    else {
      if (!isspace((unsigned char)*str)) {
        *dest++ = *str;
      }
      str++;
    }
  }

  *dest = '\0';
  return value;
}

void trimquote(char *value) {
  int n = strlen(value) - 1;

  if (isquote(value[0]) && isquote(value[n])) {
    memmove(value, value + 1, n);
    value[n - 1] = '\0';
  } else {
    n = strlen(value) - 2;

    if (isquote(value[0]) && isquote(value[n])) {
      memmove(value, value + 1, n);
      value[n - 1] = '\0';
    }
  }
}
