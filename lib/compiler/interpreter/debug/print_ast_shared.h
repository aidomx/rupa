#pragma once
#if defined(RUPA_PACKAGE_H)
/**
 * Fungsi utilitas debugging struktur AST
 * Kumpulan fungsi yang digunakan untuk membantu
 * fungsi printAst dalam membangun struktur AST.
 */
extern void printIndent(int level);
extern void printBoolean(bool value, int level);
extern void printDecimal(char *value, int level);
extern void printId(char *id, int level);
extern void printNumber(int value, int level);
extern void printNullable(char *value, int level);
extern void printString(char *value, char *label, int level);

#endif
