#include <rupa.h>

// Fungsi utilitas
void printIndent(int level) {
  for (int i = 0; i < level; i++)
    printf("  "); // Gunakan 2 spasi untuk indentasi
}

void printBoolean(bool value, int level) {
  printIndent(level);
  printf("Boolean: %s\n", value == 1 ? "true" : "false");
}

void printDecimal(char *value, int level) {
  printIndent(level);
  char *format = "Decimal: %s\n";

  printf(format, value);
}

void printId(char *id, int level) {
  printIndent(level);
  printf("Identifier: %s\n", id ? id : "null");
}

void printNumber(int value, int level) {
  printIndent(level);
  printf("Number: %d\n", value);
}

void printNullable(char *value, int level) {
  printIndent(level);
  printf("Nullable: %s\n", value);
}

void printString(char *value, char *label, int level) {
  printIndent(level);
  printf("%s: %s\n", label, value);
}
