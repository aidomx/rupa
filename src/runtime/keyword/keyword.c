#include <rupa.h>

Keyword *createKeyword() {
  Keyword *keyword = gcmall(sizeof(Keyword));
  keyword->type = KEYWORD_NONE;
  keyword->insideLoop = false;
  keyword->insideIf = false;
  if (keywordListSize > 0) {
    keyword->length = keywordListSize;
  }
  return keyword;
}

KeywordType getKeywordType(Keyword *keyword, const char *word) {
  for (int i = 0; i < keyword->length; i++) {
    if (strcmp(keywordList[i], word) == 0)
      return keywordType[i];
  }
  return KEYWORD_NONE;
}

bool isValidKeyword(char prev, char next) {
  bool left = (prev == '\0' || isspace(prev) || ispunct(prev));
  bool right = (next == '\0' || isspace(next) || ispunct(next));
  return left && right;
}

Keyword *getKeyword(Input *input) {
  if (!input || !input->keyword)
    return NULL;

  Keyword *keyword = input->keyword;
  const char *buffer = input->content;

  int start = input->cursor;
  if (isalpha(buffer[input->cursor])) {
    while (isalpha(buffer[input->cursor]))
      input->cursor++;
  }
  int end = input->cursor;

  if (end <= start)
    return NULL;

  char *word = substring(buffer, start, end);
  char prev = (start > 0) ? buffer[start - 1] : '\0';
  char next = buffer[end];

  if (!isValidKeyword(prev, next)) {
    input->cursor = start;
    return NULL;
  }

  keyword->type = getKeywordType(keyword, word);
  input->cursor = consumeToEnd(buffer, end);

  switch (keyword->type) {
  case KEYWORD_IF:
    keyword->insideIf = true;
    break;

  case KEYWORD_ELSE:
    if (!keyword->insideIf)
      return NULL;

    int lookhead = input->cursor;
    skipWhitespace(buffer, &lookhead);

    if (strncmp(buffer + lookhead, "if", 2) == 0 &&
        !isalpha(buffer[lookhead + 2])) {
      keyword->type = KEYWORD_ELSEIF;
      input->cursor = lookhead + 2;
    }
    break;

  case KEYWORD_FOR:
  case KEYWORD_REV:
  case KEYWORD_WHILE:
    keyword->insideLoop = true;
    break;

  case KEYWORD_NONE:
    input->cursor = start;
    return NULL;

  default:
    input->cursor = end;
    break;
  }

  return keyword;
}
