#include <rupa/package.h>
#include <stdbool.h>

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

int getLeftSide(Token *t, int pos) {
  int newPos = -1;

  if (isToken(t, pos, IDENTIFIER)) {
    newPos = pos;
  }

  else if (isToken(t, pos, RBLOCK)) {
    int left = findArr(t, pos);
    if (left > 0 && isToken(t, left - 1, IDENTIFIER)) {
      newPos = left - 1;
    }
  }

  return newPos;
}
