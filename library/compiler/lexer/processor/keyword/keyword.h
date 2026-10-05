#pragma once

#if defined(RUPA_PACKAGE_H)
struct Keyword {
  KeywordType type;
  int length;
  bool insideIf;
  bool insideLoop;
};

extern const char *keywordList[];
extern const int keywordListSize;
extern KeywordType keywordType[];

extern Keyword *createKeyword(void);
extern Keyword *getKeyword(Input *input);

#endif
