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
 * @brief Struktur untuk assignment operation.
 *
 * Menyimpan nama variable dan nilai yang di-assign.
 */
struct Assignment {
  char *name;
  struct AstNode *value;
};

/**
 * @brief Informasi detail untuk error reporting.
 *
 * Menyimpan pesan error, kode error, dan lokasi (line, row).
 */
struct ErrorInfo {
  char message[MAX_MESSAGE_LENGTH];
  char code[MAX_CODE_LENGTH];
  int line;
  int row;
  ErrorType type;
};

/**
 * @brief Kumpulan error information.
 *
 * Array untuk menyimpan multiple error information.
 */
struct Error {
  ErrorInfo info[256];
  int length;
};

/**
 * @brief Symbol table entry.
 *
 * Menyimpan nama symbol, value, dan lokasi deklarasi.
 */
struct SymbolTable {
  char name[64];
  char value[1024];
  int line;
  int row;
  int length;
};

/**
 * @brief Token untuk symbol characters.
 *
 * Menyimpan symbol character dan tipe tokennya.
 */
struct SymbolToken {
  char symbol;
  TokenType type;
};

/**
 * @brief Argument list untuk function calls.
 *
 * Menyimpan value argument dan informasi lokasinya.
 */
struct Args {
  char value[1024];
  int line;
  int row;
  TokenType types;
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
 * @brief Function call representation.
 *
 * Menyimpan nama fungsi, arguments, dan informasi lokasi.
 */
struct Function {
  char func[MAX_VALUE];
  char args[8][256];
  int line;
  int row;
  TokenType types;
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

/**
 * @brief System configuration settings.
 *
 * Menyimpan entry point, output format, dan buffer untuk system config.
 */
struct SystemConfig {
  char entry[MAX_BUFFER_SIZE];
  char format[64];
  char output[MAX_BUFFER_SIZE];
  int length;
};

/**
 * @brief Position range untuk token parsing.
 *
 * Merepresentasikan range token (start..end) untuk menandai posisi
 * sub-ekspresi.
 */
struct Position {
  int start;
  int end;
};

/**
 * @brief Request state untuk parsing.
 *
 * Menyimpan state parsing: node collection, token list, left position, dan
 * right range.
 */
struct Request {
  Node *node;
  struct Token *tokens;
  int left;
  Position right;
  int programId;
};

/**
 * @brief Response hasil parsing.
 *
 * Menyimpan hasil parsing berupa indeks node AST yang dihasilkan.
 */
struct Response {
  int nodeId;
  int leftId;
  int rightId;
  int nextId;
};

/**
 * @brief REPL state management.
 *
 * Menyimpan history input, length, current line, dan token list.
 */
struct ReplState {
  char **history;
  int length;
  int line;
  Token *tokens;
};

/**
 * @brief Global state untuk tokenization.
 *
 * Menyimpan REPL management, input buffer, tokens, dan current position.
 */
struct State {
  ReplState *manage;
  char input[MAX_BUFFER_SIZE];
  struct Token *tokens;
  int line;
  int row;
};
