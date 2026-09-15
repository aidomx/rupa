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
