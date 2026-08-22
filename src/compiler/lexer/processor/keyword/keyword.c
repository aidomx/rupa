#include <rupa.h>

static const char *keyword_name(KeywordType type) {
  switch (type) {
  case KEYWORD_IF:
    return "if";
  case KEYWORD_ELSEIF:
    return "elseif";
  case KEYWORD_ELSE:
    return "else";
  case KEYWORD_FOR:
    return "for";
  case KEYWORD_REV:
    return "rev";
  case KEYWORD_WHILE:
    return "while";
  case KEYWORD_PRINT:
    return "print";
  case KEYWORD_IMPORT:
    return "import";
  case KEYWORD_EXPORT:
    return "export";
  case KEYWORD_EXTENDS:
    return "extends";
  case KEYWORD_RETURN:
    return "return";
  case KEYWORD_BREAK:
    return "break";
  case KEYWORD_CONTINUE:
    return "continue";
  default:
    return NULL;
  }
}

int processKeyword(State *state, KeywordType type, int start, int next,
                   int end, bool *waiting) {
  if (!state || !state->input || !state->tokens || !waiting)
    return -1;

  const char *s = state->input->content;
  const char *name = keyword_name(type);
  if (!name)
    return -1;

  addToken(state->tokens, createDataToken((char *)name, NULL, KEYWORD,
                                          state->input->line, start));
  state->input->keyword->type = type;
  switch (type) {
  case KEYWORD_IF:
    state->input->flags->isBlockIf = true;
    break;
  case KEYWORD_ELSEIF:
    state->input->flags->isBlockElseIf = true;
    break;
  case KEYWORD_ELSE:
    state->input->flags->isBlockElse = true;
    break;
  case KEYWORD_FOR:
    state->input->flags->isFor = true;
    break;
  case KEYWORD_REV:
    state->input->flags->isRev = true;
    break;
  case KEYWORD_WHILE:
    state->input->flags->isWhile = true;
    break;
  case KEYWORD_PRINT:
    state->input->flags->isPrint = true;
    break;
  case KEYWORD_IMPORT:
    state->input->flags->isImport = true;
    break;
  case KEYWORD_EXPORT:
    state->input->flags->isExport = true;
    break;
  case KEYWORD_EXTENDS:
    state->input->flags->isExtends = true;
    break;
  case KEYWORD_RETURN:
    state->input->flags->isReturn = true;
    break;
  case KEYWORD_BREAK:
  case KEYWORD_CONTINUE:
    break;
  default:
    break;
  }

  int p = next;
  while (p < end && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r'))
    p++;

  /* Statements which require a following construct wait at EOI. */
  if (p >= end || s[p] == '\n') {
    switch (type) {
    case KEYWORD_IF:
    case KEYWORD_ELSEIF:
    case KEYWORD_FOR:
    case KEYWORD_REV:
    case KEYWORD_WHILE:
    case KEYWORD_PRINT:
    case KEYWORD_RETURN:
    case KEYWORD_IMPORT:
    case KEYWORD_EXPORT:
    case KEYWORD_EXTENDS:
      *waiting = true;
      return next;
    case KEYWORD_ELSE:
      /* `else` may be a complete branch header only when its block follows. */
      *waiting = true;
      return next;
    default:
      break;
    }
  }

  *waiting = false;
  return p;
}
