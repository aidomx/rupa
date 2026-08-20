#pragma once
#if defined(RUPA_PACKAGE_H)

struct Context {
  ContextType current;
  ContextType prev;
  int braceLevel;
  int bracketLevel;
  int parenLevel;
  int inQuotes;
  int quoteChar;
};

/**
 * @brief State context
 *
 * Menyimpan setiap karakter untuk mendukung multiline.
 */
struct StateContext {
  int brace;
  int bracket;
  int colon;
  int paren;
  int inStrictType;
  int inFunc;
  int inStruct;
  int objectDepth;
  int line;
  int lineStart;
  int space;
  int expectAssignment;
  int row;
  bool multiline;
  char *currentId;
  char *strictType;
  FlagType flagStatus;
};

#endif
