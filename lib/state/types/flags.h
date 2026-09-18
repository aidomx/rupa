#pragma once

#if defined(RUPA_PACKAGE_H)

struct Flags {
  /* Return-type annotation setelah ')' — set oleh construct lexer saat
   * menemukan `: void` / `: Type` di posisi return-type. */
  bool isReturnType;
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
  bool isCase;
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
  bool isAsync;
  bool isAwait;
  bool isComplete;
  bool isWaiting;
  ExceptType except;
};

#endif
