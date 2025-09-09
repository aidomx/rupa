#include <rupa/package.h>

bool isAssignmentStatement(Token *t, int init) {
  // Cek jika ini pattern assignment: identifier/lbracket diikuti =
  if (init + 1 >= t->length)
    return false;

  DataToken *current = getToken(t, init);
  DataToken *next = getToken(t, init + 1);

  return (match(current, IDENTIFIER) || match(current, RBLOCK)) &&
         match(next, ASSIGN);
}

bool isAssignmentToken(Token *t, int init) {
  DataToken *data = getToken(t, init);
  return match(data, ASSIGN) || isAssignmentStatement(t, init);
}

bool isPartOfAssignment(Token *t, int pos) {
  // Cek jika token ini adalah ASSIGN
  if (isToken(t, pos, ASSIGN))
    return true;

  // Cek jika token sebelum atau sesudahnya adalah ASSIGN
  if ((pos > 0 && isToken(t, pos - 1, ASSIGN)) ||
      (pos < t->length - 1 && isToken(t, pos + 1, ASSIGN))) {
    return true;
  }

  // Cek jika ini adalah identifier yang mungkin target assignment
  if (isToken(t, pos, IDENTIFIER)) {
    // Look ahead untuk assignment operator
    for (int i = pos + 1; i < t->length; i++) {
      if (isToken(t, i, ASSIGN))
        return true;
      if (!(isToken(t, i, NEWLINE) || isToken(t, i, TAB)))
        break;
    }
  }

  return false;
}

int findAssignmentOperator(Token *t, int start) {
  int end = lastIndex(t, start);
  for (int i = start; i < end; i++) {
    if (match(&t->data[i], ASSIGN)) {
      return i;
    }
  }
  return -1;
}

int findLeftHandSide(DataToken *data, int start, int end) {
  if (!data)
    return -1;
  for (int i = start; i < end; i++) {
    if (data[i].type == IDENTIFIER)
      return i;
  }
  return -1;
}

/**
 * getLeftSide: Mencari starting position untuk left-hand side expression
 * Mendukung multiple array subscripts seperti x[0][1][2]
 */
int getLeftSide(Token *t, int pos) {
  if (!t || pos < 0 || pos >= t->length)
    return -1;

  // Case 1: Simple identifier (x)
  if (isToken(t, pos, IDENTIFIER)) {
    return pos;
  }

  // Case 2: Array access (x[0], x[0][1], etc.)
  if (isToken(t, pos, RBLOCK)) {
    // Temukan bracket pembuka yang sesuai
    int lblock_pos = findArr(t, pos);
    if (lblock_pos == -1)
      return -1;

    // Sekarang kita punya: [something]
    // Cari identifier di sebelum bracket pembuka
    if (lblock_pos > 0 && isToken(t, lblock_pos - 1, IDENTIFIER)) {
      // Simple case: identifier langsung sebelum bracket (x[0])
      return lblock_pos - 1;
    } else if (lblock_pos > 0 && isToken(t, lblock_pos - 1, RBLOCK)) {
      // Nested case: another array access before (x[0][1])
      // Rekursif mencari identifier dasar
      return getLeftSide(t, lblock_pos - 1);
    }
  }

  return -1;
}
