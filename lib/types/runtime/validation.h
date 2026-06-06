#pragma once

#if defined(RUPA_PACKAGE_H)

struct ValidationInput {
  bool isArgument;
  bool isArray;
  bool isAssignment;
  bool isAnnotionType;
  bool isFunction;
  bool isFunctionDecl;
  bool isFunctionCall;
  bool isIdentifier;
  bool isBlockIf;
  bool isBlockElseIf;
  bool isBlockElse;
  bool isBlockProgram;
  bool isImport;
  bool isExport;
  bool isExtends;
  bool isFor;
  bool isRev;
  bool isStringLiteral;
  bool isStructDecl;
  bool isWhile;
  bool isPrint;
  bool isReturn;
  bool isComplete;
  bool isWaiting;
  // character
  char current;
  char prev;
  ProgramType type;
};

#endif
