#include <rupa/package.h>
#include <stdbool.h>

bool shouldSkipToken(DataToken *data, Token *t, int init) {
  switch (data->type) {
  case LBLOCK:
  case RBLOCK:
  case LPAREN:
  case RPAREN:
  case NEWLINE:
  case TAB:
    return true;

  default:
    return isPartOfAssignment(t, init);
  }
}

int findExpressionEnd(Token *t, int pos) {
  if (pos < 0 || pos >= t->length)
    return -1;

  int depth = 1, newPos = 0;
  for (int i = pos; i < t->length; i++) {
    DataToken *data = getToken(t, i);
    if (isPartOfAssignment(t, i) || shouldSkipToken(data, t, i)) {
      depth++;
    }

    else if (!isPartOfAssignment(t, i)) {
      depth--;

      if (depth == 0) {
        newPos = i;
        break;
      }
    }
  }

  return depth >= 1 ? -1 : newPos;
}

// sudah cukup stabil
int isStandaloneToken(Token *t, int pos) {
  if (pos < 0 || pos >= t->length)
    return -1;

  if (isToken(t, pos, IDENTIFIER)) {
    if (isToken(t, pos + 1, LBLOCK)) {
      return -1;
    }

    return findExpressionEnd(t, pos);
  }

  return -1;
}
