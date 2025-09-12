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

Position getTokenIndex(Token *t, int start) {
  Position pos = {-1, -1};

  if (isToken(t, start, LBLOCK)) {
    int depth = 1;

    for (int i = start + 1; i < t->length; i++) {
      if (isToken(t, i, LBLOCK))
        depth++;
      else if (isToken(t, i, RBLOCK)) {
        depth--;

        if (depth == 0) {
          pos.start = start;
          pos.end = i;

          return pos;
        }
      }
    }
  }

  else if (isToken(t, start + 1, RBLOCK)) {
    int depth = 1;

    for (int i = start + 1; i < t->length; i++) {
      if (isToken(t, i, RBLOCK)) {
        depth++;
      } else if (isToken(t, i, LBLOCK)) {
        depth--;
        if (depth == 0) {
          pos.start = start;
          pos.end = i;

          return pos;
        }
      }
    }
  }

  return pos;
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
