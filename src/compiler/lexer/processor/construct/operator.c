#include <rupa.h>

static TokenType op_type(const char *op) {
  if (!strcmp(op, "="))
    return ASSIGN;
  if (!strcmp(op, "+"))
    return PLUS;
  if (!strcmp(op, "-"))
    return MINUS;
  if (!strcmp(op, "*"))
    return STAR;
  if (!strcmp(op, "/"))
    return SLASH;
  if (!strcmp(op, "%"))
    return PERCENT;
  if (!strcmp(op, "|"))
    return PIPE;
  if (!strcmp(op, "&"))
    return AMPERSAND;
  if (!strcmp(op, "^"))
    return CARET;
  if (!strcmp(op, "~"))
    return TILDE;
  if (!strcmp(op, "?"))
    return QUESTION_MARK;
  if (!strcmp(op, "!"))
    return EXCLAMATION;
  if (!strcmp(op, "<"))
    return LESS_THAN;
  if (!strcmp(op, ">"))
    return GREATER_THAN;
  if (!strcmp(op, "."))
    return DOT;
  if (!strcmp(op, "?="))
    return QUESTION_MARK;
  if (!strcmp(op, "=="))
    return EQUAL;
  if (!strcmp(op, "!="))
    return NOT_EQUAL;
  if (!strcmp(op, "<="))
    return LESS_EQUAL;
  if (!strcmp(op, ">="))
    return GREATER_EQUAL;
  if (!strcmp(op, "&&"))
    return LOGICAL_AND;
  if (!strcmp(op, "||"))
    return LOGICAL_OR;
  if (!strcmp(op, "++"))
    return INCREMENT;
  if (!strcmp(op, "--"))
    return DECREMENT;
  if (!strcmp(op, "->"))
    return ARROW;
  if (!strcmp(op, "=>"))
    return FAT_ARROW;
  if (!strcmp(op, "<<"))
    return SHIFT_LEFT;
  if (!strcmp(op, ">>"))
    return SHIFT_RIGHT;
  if (!strcmp(op, "..."))
    return ELLIPSIS;
  return UNKNOWN;
}

int processOperator(State *state, int start, int end, int *next,
                    bool *waiting) {
  const char *s = state->input->content;
  int n = 1;
  if (start + 2 < end && s[start] == '.' && s[start + 1] == '.' &&
      s[start + 2] == '.') {
    n = 3;
  } else if (start + 1 < end) {
    char two[3] = {s[start], s[start + 1], '\0'};
    if (op_type(two) != UNKNOWN)
      n = 2;
  }

  char op[4] = {0};
  memcpy(op, s + start, n);
  TokenType type = op_type(op);
  if (type == UNKNOWN)
    type = gettype(op);
  if (type == UNKNOWN)
    return -1;

  addToken(state->tokens,
           createDataToken(op, NULL, type, state->input->line, start));
  if (type == ASSIGN || type == QUESTION_MARK)
    state->input->flags->isAssignment = true;
  *next = start + n;

  int p = *next;
  while (p < end && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r'))
    p++;
  if (p >= end || s[p] == '\n') {
    /* These operators require another expression. */
    if (type == ASSIGN || type == QUESTION_MARK || type == PLUS ||
        type == MINUS || type == STAR || type == SLASH || type == PERCENT ||
        type == PIPE || type == LOGICAL_AND || type == LOGICAL_OR ||
        type == EQUAL || type == NOT_EQUAL || type == LESS_THAN ||
        type == LESS_EQUAL || type == GREATER_THAN || type == GREATER_EQUAL ||
        type == ARROW || type == FAT_ARROW) {
      *waiting = true;
      return *next;
    }
  }
  *waiting = false;
  return 0;
}
