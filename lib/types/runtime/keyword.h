#pragma once
#if defined(RUPA_PACKAGE_H)

struct Keyword {
  KeywordType type;
  int length;
  bool insideIf;
  bool insideLoop;
};

#endif
