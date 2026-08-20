#include <rupa.h>

/*
 * Scan one identifier-shaped word and resolve it as a keyword.
 *
 * This is intentionally a lexer-level scan: no allocation and no Input cursor
 * mutation. Callers receive both the keyword type and the first position after
 * the scanned word, so the construct processor does not need to scan it again.
 */
bool scanKeyword(const char *s, int start, int end, KeywordType *type,
                 int *next) {
  if (!s || !type || !next || start < 0 || start >= end ||
      !isalpha((unsigned char)s[start]))
    return false;

  int p = start + 1;
  while (p < end && (isalnum((unsigned char)s[p]) || s[p] == '_'))
    p++;

  int length = p - start;
  if (length <= 0)
    return false;

  for (int i = 0; i < keywordListSize; i++) {
    const char *name = keywordList[i];
    if (!name || keywordType[i] == KEYWORD_NULL)
      continue;

    size_t nameLength = strlen(name);
    if ((int)nameLength == length && !strncmp(s + start, name, nameLength)) {
      *type = keywordType[i];
      *next = p;
      return true;
    }
  }

  return false;
}

KeywordType getKeywordType(Keyword *keyword, const char *word) {
  if (!keyword || !word)
    return KEYWORD_NONE;

  for (int i = 0; i < keywordListSize; i++) {
    if (keywordType[i] != KEYWORD_NULL && !strcmp(keywordList[i], word))
      return keywordType[i];
  }
  return KEYWORD_NONE;
}

bool isValidKeyword(char prev, char next) {
  bool left = (prev == '\0' || isspace((unsigned char)prev) ||
               ispunct((unsigned char)prev));
  bool right = (next == '\0' || isspace((unsigned char)next) ||
                ispunct((unsigned char)next));
  return left && right;
}

Keyword *getKeyword(Input *input) {
  if (!input || !input->keyword || !input->content)
    return NULL;

  Keyword *keyword = input->keyword;
  const char *buffer = input->content;
  int start = input->cursor;
  int end = start;
  KeywordType type = KEYWORD_NONE;

  if (!scanKeyword(buffer, start, input->length, &type, &end))
    return NULL;

  char prev = (start > 0) ? buffer[start - 1] : '\0';
  char nextChar = buffer[end];
  if (!isValidKeyword(prev, nextChar))
    return NULL;

  keyword->type = type;
  input->cursor = consumeToEnd(buffer, end);

  switch (keyword->type) {
  case KEYWORD_IF:
    keyword->insideIf = true;
    break;

  case KEYWORD_ELSE:
    if (!keyword->insideIf) {
      input->cursor = start;
      return NULL;
    }

    int lookahead = input->cursor;
    skipWhitespace(buffer, &lookahead);
    if (strncmp(buffer + lookahead, "if", 2) == 0 &&
        !isalnum((unsigned char)buffer[lookahead + 2]) &&
        buffer[lookahead + 2] != '_') {
      keyword->type = KEYWORD_ELSEIF;
      input->cursor = lookahead + 2;
    }
    break;

  case KEYWORD_FOR:
  case KEYWORD_REV:
  case KEYWORD_WHILE:
    keyword->insideLoop = true;
    break;

  default:
    input->cursor = end;
    break;
  }

  return keyword;
}
