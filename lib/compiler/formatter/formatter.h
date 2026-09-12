#pragma once
#if defined(RUPA_PACKAGE_H)

/* ==================== Formatter state ==================== */

typedef struct Formatter {
  FILE *out;
  int indent;
  bool needsIndent;
  bool lastWasNewline;
} Formatter;

/* ==================== Helpers ==================== */

extern void fmtIndent(Formatter *f);
extern void fmtStr(Formatter *f, const char *s);
extern void fmtChar(Formatter *f, char c);
extern void fmtNewline(Formatter *f);
extern void fmtSep(Formatter *f);

/* ==================== Node formatters ==================== */

extern void fmtIdentifier(Formatter *f, Node *node, int id);
extern void fmtLiteralId(Formatter *f, Node *node, int id);
extern void fmtNumber(Formatter *f, Node *node, int id);
extern void fmtDecimal(Formatter *f, Node *node, int id);
extern void fmtBoolean(Formatter *f, Node *node, int id);
extern void fmtString(Formatter *f, Node *node, int id);
extern void fmtNull(Formatter *f);

/* ==================== Expression formatters ==================== */

extern void fmtBinary(Formatter *f, Node *node, int id);
extern void fmtCall(Formatter *f, Node *node, int id);
extern void fmtPrint(Formatter *f, Node *node, int id);
extern void fmtArray(Formatter *f, Node *node, int id);
extern void fmtObject(Formatter *f, Node *node, int id);
extern void fmtMember(Formatter *f, Node *node, int id);
extern void fmtSubscript(Formatter *f, Node *node, int id);
extern void fmtUpdate(Formatter *f, Node *node, int id);
extern void fmtAnnotation(Formatter *f, Node *node, int id);
extern void fmtArrayType(Formatter *f, Node *node, int id);
extern void fmtReturn(Formatter *f, Node *node, int id);
extern void fmtThen(Formatter *f, Node *node, int id);
extern void fmtFallback(Formatter *f, Node *node, int id);

/* ==================== Statement formatters ==================== */

extern void fmtAssign(Formatter *f, Node *node, int id);
extern void fmtConditionalAssign(Formatter *f, Node *node, int id);
extern void fmtBlock(Formatter *f, Node *node, int id);
extern void fmtIf(Formatter *f, Node *node, int id);
extern void fmtLoop(Formatter *f, Node *node, int id);
extern void fmtFunctionDecl(Formatter *f, Node *node, int id);
extern void fmtStructDecl(Formatter *f, Node *node, int id);
extern void fmtModule(Formatter *f, Node *node, int id);
extern void fmtModuleImport(Formatter *f, Node *node, int id);
extern void fmtExport(Formatter *f, Node *node, int id);
extern void fmtAsync(Formatter *f, Node *node, int id);
extern void fmtCase(Formatter *f, Node *node, int id);

/* ==================== Dispatch ==================== */

extern void fmtNode(Formatter *f, Node *node, int id);

/* ==================== Comment formatting ==================== */

extern void fmtSingleComment(Formatter *f, const char *text, int len);
extern void fmtBlockComment(Formatter *f, const char *text, int len);
extern void formatComment(const char *source, int *pos, int length);

/* ==================== Entry points ==================== */

extern int formatFile(const char *path);
extern int formatString(const char *source);
extern int formatStdin(void);

#endif
