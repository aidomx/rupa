#pragma once
#if defined(RUPA_PACKAGE_H)

/* ==================== Formatter state ==================== */

typedef struct FormatterConfig {
  int indentWidth;
  int maxEmpty;
  int objectLimit;
  bool objectCollapse;
  bool objectSpaceTrim;
  bool keepEmptyBlock;
  bool classKeepEmptyBlock;
  bool classMemberKeepEmptyBlock;
  bool classMemberKeepEmptyMember;
} FormatterConfig;

typedef struct Formatter {
  FILE *out;
  int indent;
  bool needsIndent;
  bool lastWasNewline;
  int pendingNewline; /* newline tertunda: komentar inline boleh menempel */
  const FormatterConfig *config;
  bool inClassMembers;
} Formatter;

/* Daftar path .rp yang dikumpulkan untuk mode batch (format_paths.c). */
typedef struct FmtPathList {
  char **items;
  int count;
  int capacity;
} FmtPathList;

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
extern void fmtStringInterp(Formatter *f, Node *node, int id);
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
extern void fmtEnumDecl(Formatter *f, Node *node, int id);
extern void fmtClassDecl(Formatter *f, Node *node, int id);
extern void fmtMod(Formatter *f, Node *node, int id);
extern void fmtAsync(Formatter *f, Node *node, int id);
extern void fmtCase(Formatter *f, Node *node, int id);

/* ==================== Dispatch ==================== */

extern void fmtNode(Formatter *f, Node *node, int id);

/* Blank-line preservation di dalam body blok (class/struct). */
extern int fmtNodeEndLine(Node *node, int id);
extern void fmtMemberGap(Formatter *f, Node *node, int prevId, int nextId);

/* ==================== Comment formatting ==================== */

extern void fmtSingleComment(Formatter *f, const char *text, int len);
extern void fmtBlockComment(Formatter *f, const char *text, int len);
extern void formatCommentTo(FILE *out, const char *source, int *pos, int length);
extern void formatComment(const char *source, int *pos, int length);

/* ==================== Entry points ==================== */

extern int formatFile(const char *path);
extern int formatString(const char *source);
extern int formatStdin(void);
extern int formatList(const char *path, bool listOnly);
extern int formatSelect(const char *select, const char *path, const char **excludes,
                        int excludeCount);

/* ==================== Config (format_config.c) ==================== */

extern FormatterConfig fmtLoadConfig(void);

/* ==================== Source normalization (format_normalize.c) ========= */

extern bool fmtSourceConfigCompliant(const char *src, size_t len, const FormatterConfig *c);
extern char *fmtNormalizeSource(const char *src, size_t len, const FormatterConfig *c,
                                size_t *outLen);

/* ==================== Format engine (format_engine.c) =================== */

extern int runFormat(State *state, Formatter *fmt);

/* ==================== Path utilities (format_paths.c) =================== */

extern void fmtPathListFree(FmtPathList *list);
extern int fmtPathCmp(const void *a, const void *b);
extern int fmtCollectRp(const char *root, FmtPathList *list);
extern bool fmtPathExcluded(const char *path, const char **excludes, int count);
extern int fmtResolveTestPath(const char *path, char *out, size_t outSize);
extern void fmtPrintList(const FmtPathList *list, const char **excludes, int excludeCount);
extern char *fmtReadSource(const char *path);

/* ==================== Batch orchestration (format_batch.c) ============== */

extern int fmtRunFile(const char *path, FILE *out);

#endif
