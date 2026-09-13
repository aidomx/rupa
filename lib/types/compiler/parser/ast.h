#pragma once

#if defined(RUPA_PACKAGE_H)
// ==================== STRUCT DEFINITIONS ====================

struct AstArray {
  int *elements;
  int length;
};

struct AstArrayType {
  int elementType;
};

/**
 * @brief Representasi AST untuk assignment statement.
 *
 * Menyimpan target dan value dari suatu assignment operation.
 */
struct AstAssignment {
  int target;
  int type; // -1 jika tidak ada explicit type annotation
  int value;
};

/* Comment */
struct AstComment {
  int type; // NODE_INLINE_COMMENT OR NODE_BLOCK_COMMENT
  char *value;
};

/**
 * @brief Representasi AST untuk binary operation.
 *
 * Menyimpan operator dan operand kiri/kanan dari binary expression.
 */

struct AstConditionalAssignment {
  int target;
  int value;
};

struct AstThen {
  int condition;
  int result;
};

struct AstFallback {
  int primary;
  int fallback;
};

struct AstAsync {
  int request;
  int handler;   // -1 jika tidak ada handler (legacy block)
  int timeout;   // -1 jika tidak ada timeout value node (legacy)
  int loaderId;  // -1 jika tidak ada loader (identifier reference)
  int timeoutId; // -1 jika tidak ada timeout (identifier reference)
};

struct AstAwait {
  int expression;
};

struct AstMember {
  int object;
  int member;
};

struct AstBinary {
  enum BinaryType type;
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
  enum BinaryType type;
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
 * @brief Representasi literal decimal tanpa menentukan precision runtime.
 */
struct AstDecimal {
  char *lexeme;
  double value; // cache numerik; float/double runtime ditentukan kemudian
};

/**
 * @brief Representasi AST untuk identifier.
 *
 * Menyimpan nama identifier (variabel, fungsi, dll).
 */
struct AstIdentifier {
  char *name;
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
  struct AstDeclaration *declarations;
};

/**
 * @brief Representasi AST untuk return statement.
 *
 * Menyimpan expression yang akan dikembalikan oleh fungsi.
 */
struct AstReturn {
  int expression;
  bool explicitReturn; /* true for `return ...`, false for implicit expression statements */
};

/**
 * @brief Representasi AST untuk string literal.
 *
 * Menyimpan tipe token dan nilai string.
 */
struct AstString {
  enum TokenType type;
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
  enum VariableType type;
  struct AstNode *name;
  struct AstNode *next;
};

struct AstList {
  int *items;
  int length;
};

struct AstCall {
  int callee;
  int *args;
  int length;
};

struct AstPrint {
  int *args;
  int length;
};

struct AstBlock {
  int *statements;
  int length;
};

struct AstIf {
  int condition;
  int thenBlock;
  int elseBlock;
  bool isBlock;
};

struct AstLoop {
  char *kind;
  int condition;
  int body;
};

struct AstFunctionDecl {
  int name;
  int *params;
  int paramLength;
  int body;
};

struct AstStructDecl {
  int name;
  int body;
};

struct AstAnnotation {
  int name;
  int type;
  int value;
};

/**
 * Entry serbaguna module: daftar import/export, alias, dan policy.
 *
 * Semua string via gcstrdup, array via gccalloc — dikelola GC,
 * tidak ada free manual.
 *
 * import x from Y            → { MOD_ID, name:"x" }
 * import x as y from Y       → { MOD_ID, name:"x", key:"y" }
 * import a.create from Y     → { MOD_MEMBER, name:"a", childrens:{name:"create"} }
 * import d.* from Y          → { MOD_WILD, name:"d" }
 * export c from Y -> { a: private }
 *   policies: { name:"a", value:"private" }
 */
struct AstModEntry {
  enum ModEntryKind type;
  char *name;
  char *key;                     /* binding lokal `x as y` → "y"; NULL = pakai name */
  char *value;                   /* policy `{ a: private }` → "private"; selain itu NULL */
  struct AstModEntry *childrens; /* sub-path a.create → rantai entry */
};

/**
 * 1 container untuk 2 job (import & export).
 *
 * import x from Y            → type=ImportDecl, entries=[x], source="Y"
 * import x as y from Y       → entries=[{name:"x", key:"y"}]
 * import a.create from Y     → entries=[{MOD_MEMBER, name:"a", childrens:[create]}]
 * import d.* from Y as m     → entries=[{MOD_WILD, name:"d"}], sourceAlias="m"
 * export x                   → type=ExportDecl, entries=[x], source=NULL (kontrak)
 * export c from Y -> { a: private }
 *                            → policies=[{name:"a", value:"private"}]
 */
struct AstMod {
  enum ModType type;           /* ImportDecl / ExportDecl */
  struct AstModEntry *entries; /* gccalloc(entryCount * sizeof(AstModEntry)) */
  int entryCount;
  char *source;                /* "../modules", "rupa.os"; NULL = export lokal */
  char *sourceAlias;           /* `from X as m` → "m"; NULL */
  struct AstModEntry *policies;/* `-> { a: private }`; NULL */
  int policyCount;
  int body; /* NamespaceDecl only: NODE_BLOCK id holding nested ExportDecl
             * statements (`namespace db { export ...; export ...; }`).
             * -1 for ImportDecl/ExportDecl. */
};

struct AstModule {
  int value;
  int name; // submodule index (-1 if not used, e.g. import X from rupa)
};

struct AstUpdate {
  int target;
  char *op;
  bool prefix;
};

struct AstObjectEntry {
  int key;
  int value;
};

struct AstObject {
  struct AstObjectEntry *entries;
  int length;
};

struct AstCaseEntry {
  int pattern; /* -1 means wildcard */
  int body;
  bool wildcard;
};

struct AstCase {
  int subject;
  struct AstCaseEntry *entries;
  int length;
};

struct AstMemberAssign {
  int target; /* NODE_MEMBER or NODE_SUBSCRIPT node id */
  int value;  /* expression node id */
};

/**
 * String interpolation: "Hello {{name}}, you have {{count}} items\n"
 * Parts array alternates between NODE_STRING (literal) and expression nodes.
 */
struct AstStringInterp {
  int *parts; /* array of node IDs */
  int length; /* number of parts */
};

/**
 * @brief Abstract Syntax Tree node.
 *
 * Union yang dapat menyimpan berbagai jenis AST node types.
 */
struct AstNode {
  enum NodeType type;
  int line;
  int row;
  struct DataToken *token;
  union {
    struct AstArray array;
    struct AstArrayType arrayType;
    struct AstAssignment assign; ///< Assignment operation
    struct AstConditionalAssignment conditionalAssign;
    struct AstThen then;
    struct AstFallback fallback;
    struct AstAsync async;
    struct AstAwait await;
    struct AstMember member;
    struct AstBinary binary;                     ///< Binary operation
    struct AstBinaryExpression binaryExpression; ///< Binary expression
    struct AstBoolean boolean;                   ///< Boolean literal
    struct AstComment asComment;                 // comment
    struct AstDouble asDouble;                   ///< Double precision number
    struct AstDecimal decimal;                   ///< Decimal literal; runtime type resolved later
    struct AstIdentifier identifier;             ///< Identifier reference
    struct AstNumber number;                     ///< Integer number
    struct AstProgram program;                   ///< Program root node
    struct AstReturn asReturn;                   ///< Return statement
    struct AstString string;                     ///< String literal
    struct AstSubscript subscript;               ///< Array subscript
    struct AstVariable variable;                 ///< Variable declaration
    struct AstCall call;
    struct AstPrint print;
    struct AstBlock block;
    struct AstIf asIf;
    struct AstLoop loop;
    struct AstFunctionDecl function;
    struct AstStructDecl asStruct;
    struct AstAnnotation annotation;
    struct AstMod mod;
    struct AstModule module;
    struct AstObject object;
    struct AstCase asCase;
    struct AstUpdate update;
    struct AstMemberAssign memberAssign;
    struct AstStringInterp stringInterp;
  };
};

#endif
