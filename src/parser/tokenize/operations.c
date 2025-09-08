#include <rupa/package.h>

int addDelim(Token *tokens, char c, char *safetyType, int line, int row) {
  if (!tokens)
    return -1;

  char del[2] = {c, '\0'};
  DataToken data = {
      .line = line, .row = row, .safetyType = safetyType, .value = strdup(del)};

  if (isassign(c)) {
    data.type = ASSIGN;
    return addToken(tokens, data);
  }

  switch (c) {
  case '+':
    data.type = PLUS;
    break;
  case '-':
    data.type = MINUS;
    break;
  case '*':
    data.type = STAR;
    break;
  case '/':
    data.type = SLASH;
    break;
  case '%':
    data.type = PERCENT;
    break;
  case '&':
    data.type = AMPERSAND;
    break;
  case '|':
    data.type = PIPE;
    break;
  case '^':
    data.type = CARET;
    break;
  case '~':
    data.type = TILDE;
    break;
  case '?':
    data.type = QUESTION_MARK;
    break;
  case ':':
    data.type = COLON;
    break;
  case '.':
    data.type = DOT;
    break;
  case ',':
    data.type = COMMA;
    break;
  case ';':
    data.type = SEMICOLON;
    break;
  case '@':
    data.type = AT;
    break;
  case '$':
    data.type = DOLLAR;
    break;
  case '!':
    data.type = EXCLAMATION;
    break;
  case '<':
    data.type = LESS_THAN;
    break;
  case '>':
    data.type = GREATER_THAN;
    break;
  case '=':
    data.type = EQUAL_THAN;
    break;
  case '#':
    data.type = HASHTAG;
    break;
  case '\\':
    data.type = BACKSLASH;
    break;
  case '`':
    data.type = BACKTICK;
    break;
  case '"':
    data.type = QUOTE;
    break;
  case '\'':
    data.type = SINGLE_QUOTE;
    break;
  case '[':
    data.type = LBLOCK;
    break;
  case ']':
    data.type = RBLOCK;
    break;
  case '{':
    data.type = LBRACE;
    break;
  case '}':
    data.type = RBRACE;
    break;
  case '(':
    data.type = LPAREN;
    break;
  case ')':
    data.type = RPAREN;
    break;
  case '\n':
    data.type = NEWLINE;
    break;
  case '\t':
    data.type = TAB;
    break;
  case '\r':
    data.type = CARRIAGE_RETURN;
    break;
  case '\b':
    data.type = BACKSPACE;
    break;
  case '\f':
    data.type = FORM_FEED;
    break;
  default:
    data.type = UNKNOWN;
    break;
  }

  return addToken(tokens, data);
}
