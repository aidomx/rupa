#include <rupa.h>

Flags *createFlags(size_t size) {
  if (!gc)
    return NULL;

  Flags *flags = gcmall(size);

  // Expression / Definition flags
  // identifier tunggal
  flags->isIdentifier = false;
  // assignment biasa
  flags->isAssignment = false;
  // assignment dengan type annotion
  flags->isAnnotionType = false;
  // argument
  flags->isArgument = false;
  // assignment dengan subscript
  // x[] = ...
  flags->isArray = false;
  // Mendeteksi fungsi
  flags->isFunction = false;
  // pemanggilan fungsi
  flags->isFunctionCall = false;
  // deklarasi fungsi
  flags->isFunctionDecl = false;
  // deklarasi string literal
  flags->isStringLiteral = false;
  // deklarasi struct
  flags->isStructDecl = false;
  // deklarasi identifier list
  flags->isExceptIdentifier = false;
  // extends untuk turunan struct dari file lain
  flags->isExtends = false;

  // Control flow / Block structures
  // block if
  flags->isBlockIf = false;
  // block else if
  flags->isBlockElseIf = false;
  // block else
  flags->isBlockElse = false;
  // block start program : {
  flags->isBlockProgram = false;
  // loop for
  flags->isFor = false;
  // loop reverse
  flags->isRev = false;
  // loop while
  flags->isWhile = false;
  // return statement
  flags->isReturn = false;

  // subscript []
  flags->isSubs = false;

  // Module / I/O
  // import modul
  flags->isImport = false;
  // export modul
  flags->isExport = false;
  // fungsi cetak
  flags->isPrint = false;

  // Final / State
  flags->isComplete = false;
  flags->isWaiting = false;

  // error list
  flags->isBroken = false;
  flags->isInvalid = false;

  // except
  flags->except = EXCEPT_PROGRAM;

  return flags;
}

void resetFlags(Flags *flags) {
  if (!flags)
    return;

  if (flags->isComplete)
    memset(flags, 0, sizeof(Flags));
}
