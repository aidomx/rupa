#ifndef RUPA_IR_REWRITE_INTERNAL_H
#define RUPA_IR_REWRITE_INTERNAL_H

#include <rupa.h>

/*
 * rewrite_internal.h — kontrak internal antar-unit rewrite (AST -> IR).
 *
 * rewrite.c dipecah menjadi unit kecil (rules.md: modular, jangan
 * membengkak) yang berbagi IRBuilder:
 *   rewrite_scope.c    — scope map (binding nama -> slot IR)
 *   rewrite_builder.c  — core builder: cache, type helper, fold, blok baru
 *   rewrite_flow.c     — control flow: if/loop/case + walker penggunaan id
 *   rewrite_expr.c     — buildNode (ekspresi) + pipeSegments
 *   rewrite_stmt.c     — statement dispatcher + build program/fungsi/assign
 *   rewrite.c          — entry point rewrite()
 *
 * Semua unit include <rupa.h> + header ini; tidak ada state selain
 * g_loop (rewrite_flow.c).
 */

typedef struct IRBuilder IRBuilder;
typedef struct ScopeMap ScopeMap;

struct IRBuilder {
  Node *astRef; /* pool AST sumber */
  IRModule *module;
  IRFunction *function;
  IRBlock *block;
  /* Cache operand berdasarkan AST node id supaya subexpression yang
   * sama tidak diturunkan dua kali. */
  IRValue **ops;
  int opCap;
  int opLen;
  /* Nama variable -> IRValue slot (local). Lintas scope dengan marker. */
  ScopeMap *scopes;
};

struct ScopeMap {
  char *name;
  IRValue *value;
  char *type; /* type deklarasi (x: T = ...) — kontrak reassignment */
  ScopeMap *next;
};

/* Konteks loop untuk break/continue (state global di rewrite_flow.c). */
typedef struct LoopCtx {
  IRBuilder *b;
  IRBlock *breakTarget;
  IRBlock *continueTarget;
  struct LoopCtx *parent;
} LoopCtx;

/* ==================== rewrite_scope.c ==================== */

IRValue *scopeFind(IRBuilder *b, const char *name);
const char *scopeTypeOf(IRBuilder *b, const char *name);
void scopeBind(IRBuilder *b, const char *name, IRValue *value);
void scopeSetType(IRBuilder *b, const char *name, const char *type);
void scopePush(IRBuilder *b);
void scopePop(IRBuilder *b);

/* ==================== rewrite_builder.c ==================== */

void builderInit(IRBuilder *b, Node *ast, IRModule *module);
IRValue *cacheOp(IRBuilder *b, int id, IRValue *value);
const char *nodeName(Node *node, int id);
IRType *numberType(void);
IRType *boolType(void);
IRType *nullType(void);
IROpcode opcodeFor(const char *op);
int irNumericType(IRValue *v);
IRValue *foldBinary(IROpcode op, IRValue *l, IRValue *r);
IRValue *emitBinary(IRBuilder *b, IROpcode op, IRValue *l, IRValue *r);
IRValue *newLocal(IRBuilder *b, const char *name);
IRBlock *newBlock(IRBuilder *b, const char *prefix);
IRValue *slotValue(IRValue *slot);
const char *annotationTypeName(IRBuilder *b, int typeId);
bool memoryContractIsContract(Node *node, int valueId);

/* ==================== rewrite_flow.c ==================== */

void buildBreak(void);
void buildContinue(void);
bool astUsesIdentifier(const Node *ast, int id, const char *name);
void buildIf(IRBuilder *b, Node *node, const AstNode *a);
void buildLoop(IRBuilder *b, Node *node, const AstNode *a);
void buildCase(IRBuilder *b, const AstNode *a);

/* ==================== rewrite_expr.c ==================== */

IRValue *buildNode(IRBuilder *b, int id);
int pipeSegments(Node *node, int id, int *segs, int cap);

/* ==================== rewrite_stmt.c ==================== */

void buildProgram(IRBuilder *b, Node *node, int root);
void buildFunctionDecl(IRBuilder *b, Node *node, const AstNode *a);
void buildAssign(IRBuilder *b, Node *node, const AstNode *a, int id);
void buildConditionalAssign(IRBuilder *b, Node *node, const AstNode *a);
void buildAnnotation(IRBuilder *b, Node *node, const AstNode *a);
void buildPrint(IRBuilder *b, Node *node, const AstNode *a);
void buildReturn(IRBuilder *b, Node *node, const AstNode *a);
void buildBlock(IRBuilder *b, int id);
IRValue *buildStatementValue(IRBuilder *b, int id);

#endif /* RUPA_IR_REWRITE_INTERNAL_H */
