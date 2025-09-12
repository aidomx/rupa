#pragma once

#include "rupa/package.h"

/**
 * @brief Struktur untuk assignment operation.
 *
 * Menyimpan nama variable dan nilai yang di-assign.
 */
struct Assignment {
  char *name;
  struct AstNode *value;
};

/**
 * @brief Binary operation structure.
 *
 * Menyimpan operator dan operand kiri/kanan.
 */
struct Binary {
  char *op;
  struct AstNode *left;
  struct AstNode *right;
};

/**
 * @brief Identifier structure.
 *
 * Menyimpan nama identifier.
 */
struct Identifier {
  char *name;
};

/**
 * @brief Number structure.
 *
 * Menyimpan nilai numerik.
 */
struct Number {
  int value;
};

/**
 * @brief Dynamic array untuk AST nodes.
 *
 * Menyimpan array of AST nodes dengan capacity dan length.
 */
struct Node {
  AstNode *ast;
  int capacity;
  int length;
};
