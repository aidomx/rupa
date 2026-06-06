#include <rupa.h>

ValidationInput *createValidationInput(size_t size) {
  if (!gc)
    return NULL;

  ValidationInput *vi = gcmall(size);

  // Expression / Definition flags
  // identifier tunggal
  vi->isIdentifier = false;
  // assignment biasa
  vi->isAssignment = false;
  // assignment dengan type annotion
  vi->isAnnotionType = false;
  // argument
  vi->isArgument = false;
  // assignment dengan subscript
  // x[] = ...
  vi->isArray = false;
  // Mendeteksi fungsi
  vi->isFunction = false;
  // pemanggilan fungsi
  vi->isFunctionCall = false;
  // deklarasi fungsi
  vi->isFunctionDecl = false;
  // deklarasi string literal
  vi->isStringLiteral = false;
  // deklarasi struct
  vi->isStructDecl = false;
  // extends untuk turunan struct dari file lain
  vi->isExtends = false;

  // Control flow / Block structures
  // block if
  vi->isBlockIf = false;
  // block else if
  vi->isBlockElseIf = false;
  // block else
  vi->isBlockElse = false;
  // block start program : {
  vi->isBlockProgram = false;
  // loop for
  vi->isFor = false;
  // loop reverse
  vi->isRev = false;
  // loop while
  vi->isWhile = false;
  // return statement
  vi->isReturn = false;

  // Module / I/O
  // import modul
  vi->isImport = false;
  // export modul
  vi->isExport = false;
  // fungsi cetak
  vi->isPrint = false;

  // Final / State
  vi->isComplete = false;
  vi->isWaiting = false;

  // character
  vi->current = '\0';
  vi->prev = '\0';

  // state idle
  vi->type = PROGRAM_IDLE;

  return vi;
}
