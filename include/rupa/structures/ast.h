#pragma once

#include "rupa/package.h"
// ==================== STRUCT DEFINITIONS ====================

struct AstArray {
  int *elements;
  int length;
};

/**
 * @brief Representasi AST untuk assignment statement.
 *
 * Menyimpan target dan value dari suatu assignment operation.
 */
struct AstAssignment {
  int target;
  int value;
};

/**
 * @brief Representasi AST untuk binary operation.
 *
 * Menyimpan operator dan operand kiri/kanan dari binary expression.
 */
struct AstBinary {
  BinaryType type;
  char *op;
  int left;
  int right;
};

/**
 * @brief Representasi AST untuk binary expression.
 *
 * Digunakan untuk menyimpan informasi operator dalam expression.
 */
struct AstBinaryExpression {
  BinaryType type;
  char *op;
};

/**
 * @brief Representasi AST untuk boolean literal.
 *
 * Menyimpan nilai boolean (true/false).
 */
struct AstBoolean {
  bool value;
};

/**
 * @brief Linked list untuk deklarasi dalam program.
 *
 * Menyimpan daftar node ID deklarasi dalam bentuk linked list.
 */
struct AstDeclaration {
  int nodeId;
  struct AstDeclaration *next;
};

/**
 * @brief Representasi AST untuk double precision number.
 *
 * Menyimpan nilai floating-point dengan presisi ganda.
 */
struct AstDouble {
  double value;
};

/**
 * @brief Representasi AST untuk floating-point number.
 *
 * Menyimpan lexeme dan nilai float.
 */
struct AstFloat {
  char *lexeme;
  float value;
};

/**
 * @brief Representasi AST untuk identifier.
 *
 * Menyimpan nama identifier (variabel, fungsi, dll).
 */
struct AstIdentifier {
  char *name;
  char *safetyType;
};

/**
 * @brief Representasi AST untuk integer number.
 *
 * Menyimpan nilai integer.
 */
struct AstNumber {
  int value;
};

/**
 * @brief Representasi AST untuk program root.
 *
 * Menyimpan linked list deklarasi yang membentuk suatu program.
 */
struct AstProgram {
  AstDeclaration *declarations;
};

/**
 * @brief Representasi AST untuk return statement.
 *
 * Menyimpan expression yang akan dikembalikan oleh fungsi.
 */
struct AstReturn {
  int expression;
};

/**
 * @brief Representasi AST untuk string literal.
 *
 * Menyimpan tipe token dan nilai string.
 */
struct AstString {
  TokenType type;
  char *value;
};

/**
 * @brief Representasi AST untuk array subscript.
 *
 * Digunakan untuk operasi indexing pada array (e.g., arr[index]).
 */
struct AstSubscript {
  int posId;
  int index;
};

/**
 * @brief Representasi AST untuk variable declaration.
 *
 * Menyimpan tipe variable, nama, dan pointer ke next declaration.
 */
struct AstVariable {
  VariableType type;
  struct AstNode *name;
  struct AstNode *next;
};

/**
 * @brief Abstract Syntax Tree node.
 *
 * Union yang dapat menyimpan berbagai jenis AST node types.
 */
struct AstNode {
  NodeType type;
  union {
    AstArray array;
    AstAssignment assign;                 ///< Assignment operation
    AstBinary binary;                     ///< Binary operation
    AstBinaryExpression binaryExpression; ///< Binary expression
    AstBoolean boolean;                   ///< Boolean literal
    AstDouble asDouble;                   ///< Double precision number
    AstFloat asFloat;                     ///< Floating-point number
    AstIdentifier identifier;             ///< Identifier reference
    AstNumber number;                     ///< Integer number
    AstProgram program;                   ///< Program root node
    AstReturn asReturn;                   ///< Return statement
    AstString string;                     ///< String literal
    AstSubscript subscript;               ///< Array subscript
    AstVariable variable;                 ///< Variable declaration
    DataToken *token;                     ///< Raw token data
  };
};
