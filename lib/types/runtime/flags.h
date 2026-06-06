#pragma once

#if defined(RUPA_PACKAGE_H)

struct Flags {
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
  bool isBroken;
  bool isImport;
  bool isInvalid;
  bool isExport;
  bool isExceptIdentifier;
  bool isExtends;
  bool isFor;
  bool isRev;
  bool isStringLiteral;
  bool isStructDecl;
  bool isSubs;
  bool isWhile;
  bool isPrint;
  bool isReturn;
  bool isComplete;
  bool isWaiting;
  ExceptType except;
};

#endif
