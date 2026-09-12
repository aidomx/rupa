#include <rupa.h>

/* ---- Module design baru (NODE_MOD) — 1 container untuk import & export ----
 * Semua alokasi via GC (gccalloc/gcstrdup), tidak ada free manual.
 */

AstModEntry *modEntry(const char *name) {
  AstModEntry *e = gccalloc(1, sizeof(AstModEntry));
  if (!e) return NULL;
  e->type = MOD_ID;
  e->name = gcstrdup(name);
  return e;
}

AstModEntry *modEntryKey(const char *name, const char *key) {
  AstModEntry *e = modEntry(name);
  if (!e) return NULL;
  e->key = gcstrdup(key);
  return e;
}

AstModEntry *modEntryWild(const char *name) {
  AstModEntry *e = modEntry(name);
  if (!e) return NULL;
  e->type = MOD_WILD;
  return e;
}

AstModEntry *modEntryMember(const char *name, AstModEntry *path) {
  AstModEntry *e = modEntry(name);
  if (!e) return NULL;
  e->type = MOD_MEMBER;
  e->childrens = path;
  return e;
}

AstModEntry *modPolicy(const char *name, const char *value) {
  AstModEntry *e = modEntry(name);
  if (!e) return NULL;
  e->value = gcstrdup(value);
  return e;
}

/* Copy array entry ke GC — struktur rantai childrens ikut diduplikat. */
static AstModEntry *modCopyEntries(AstModEntry *entries, int entryCount) {
  if (!entries || entryCount <= 0) return NULL;
  AstModEntry *out = gccalloc(entryCount, sizeof(AstModEntry));
  if (!out) return NULL;
  for (int i = 0; i < entryCount; i++) {
    out[i].type = entries[i].type;
    out[i].name = entries[i].name ? gcstrdup(entries[i].name) : NULL;
    out[i].key = entries[i].key ? gcstrdup(entries[i].key) : NULL;
    out[i].value = entries[i].value ? gcstrdup(entries[i].value) : NULL;
    /* Rantai childrens disalin rekursif (shallow per node, GC-owned). */
    AstModEntry *src = entries[i].childrens;
    AstModEntry *dst = NULL;
    while (src) {
      AstModEntry *c = gccalloc(1, sizeof(AstModEntry));
      if (!c) break;
      c->type = src->type;
      c->name = src->name ? gcstrdup(src->name) : NULL;
      c->key = src->key ? gcstrdup(src->key) : NULL;
      c->value = src->value ? gcstrdup(src->value) : NULL;
      if (dst)
        dst->childrens = c;
      else
        out[i].childrens = c;
      dst = c;
      src = src->childrens;
    }
  }
  return out;
}

int createModImport(Node *root, AstModEntry *entries, int entryCount,
                    const char *source, const char *sourceAlias) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MOD};
  n.mod.type = ImportDecl;
  n.mod.entries = modCopyEntries(entries, entryCount);
  n.mod.entryCount = entryCount;
  n.mod.source = source ? gcstrdup(source) : NULL;
  n.mod.sourceAlias = sourceAlias ? gcstrdup(sourceAlias) : NULL;
  n.mod.policies = NULL;
  n.mod.policyCount = 0;
  return createAst(root, n);
}

int createModExport(Node *root, AstModEntry *entries, int entryCount,
                    const char *source, AstModEntry *policies, int policyCount) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MOD};
  n.mod.type = ExportDecl;
  n.mod.entries = modCopyEntries(entries, entryCount);
  n.mod.entryCount = entryCount;
  n.mod.source = source ? gcstrdup(source) : NULL;
  n.mod.sourceAlias = NULL;
  n.mod.policies = modCopyEntries(policies, policyCount);
  n.mod.policyCount = policies ? policyCount : 0;
  return createAst(root, n);
}

/* AST Node Factory — Statements, Module, Async, Case
 * Core factory functions remain in factory.c.
 * createAst and copyIds are declared in rupa_modules.h
 */
int createComment(Node *root, char *value, int type) {
  if (!root) return -1;
  AstNode n = {.type = NODE_COMMENT};
  n.asComment.type = type;
  n.asComment.value = strdup(value);
  return createAst(root, n);
}

int createConditionalAssignment(Node *root, int target, int value) {
  if (!root || target < 0 || value < 0) return -1;
  AstNode node = {.type = NODE_CONDITIONAL_ASSIGN,
                  .conditionalAssign = {.target = target, .value = value}};
  return createAst(root, node);
}

int createThen(Node *root, int condition, int result) {
  if (!root || condition < 0 || result < 0) return -1;
  AstNode node = {.type = NODE_THEN, .then = {.condition = condition, .result = result}};
  return createAst(root, node);
}

int createFallback(Node *root, int primary, int fallback) {
  if (!root || primary < 0 || fallback < 0) return -1;
  AstNode node = {.type = NODE_FALLBACK, .fallback = {.primary = primary, .fallback = fallback}};
  return createAst(root, node);
}

int createCall(Node *root, int callee, int *args, int length) {
  AstNode n = {.type = NODE_CALL};
  n.call.callee = callee;
  n.call.args = copyIds(args, length);
  n.call.length = length;
  return createAst(root, n);
}

int createPrint(Node *root, int *args, int length) {
  AstNode n = {.type = NODE_PRINT};
  n.print.args = copyIds(args, length);
  n.print.length = length;
  return createAst(root, n);
}

int createBlock(Node *root, int *items, int length) {
  AstNode n = {.type = NODE_BLOCK};
  n.block.statements = copyIds(items, length);
  n.block.length = length;
  return createAst(root, n);
}

int createIf(Node *root, int condition, int thenBlock, int elseBlock) {
  AstNode n = {.type = NODE_IF};
  n.asIf.condition = condition;
  n.asIf.thenBlock = thenBlock;
  n.asIf.elseBlock = elseBlock;
  return createAst(root, n);
}

int createLoop(Node *root, const char *kind, int condition, int body) {
  AstNode n = {.type = NODE_LOOP};
  n.loop.kind = gcstrdup(kind ? kind : "");
  n.loop.condition = condition;
  n.loop.body = body;
  return createAst(root, n);
}

int createFunctionDecl(Node *root, int name, int *params, int paramLength, int body) {
  AstNode n = {.type = NODE_FUNCTION_DECL};
  n.function.name = name;
  n.function.params = copyIds(params, paramLength);
  n.function.paramLength = paramLength;
  n.function.body = body;
  return createAst(root, n);
}

int createStructDecl(Node *root, int name, int body) {
  AstNode n = {.type = NODE_STRUCT_DECL};
  n.asStruct.name = name;
  n.asStruct.body = body;
  return createAst(root, n);
}

int createAnnotation(Node *root, int name, int type, int value) {
  AstNode n = {.type = NODE_ANNOTATION};
  n.annotation.name = name;
  n.annotation.type = type;
  n.annotation.value = value;
  return createAst(root, n);
}

int createModule(Node *root, NodeType type, int value, int name) {
  AstNode n = {.type = type};
  n.module.value = value;
  n.module.name = name;
  return createAst(root, n);
}

int createObject(Node *root, struct AstObjectEntry *entries, int length) {
  AstNode n = {.type = NODE_OBJECT};
  if (length > 0) {
    n.object.entries = gcmall(sizeof(struct AstObjectEntry) * length);
    if (!n.object.entries) return -1;
    memcpy(n.object.entries, entries, sizeof(struct AstObjectEntry) * length);
  }
  n.object.length = length;
  return createAst(root, n);
}

int createAsync(Node *root, int request, int handler, int timeout, int loaderId, int timeoutId) {
  if (!root || request < 0) return -1;
  AstNode node = {.type = NODE_ASYNC,
                  .async = {.request = request,
                            .handler = handler,
                            .timeout = timeout,
                            .loaderId = loaderId,
                            .timeoutId = timeoutId}};
  return createAst(root, node);
}

int createAwait(Node *root, int expression) {
  if (!root || expression < 0) return -1;
  AstNode node = {.type = NODE_AWAIT, .await = {.expression = expression}};
  return createAst(root, node);
}

int createMember(Node *root, int object, int member) {
  if (!root || object < 0 || member < 0) return -1;
  AstNode node = {.type = NODE_MEMBER, .member = {.object = object, .member = member}};
  return createAst(root, node);
}

int createCase(Node *root, int subject, struct AstCaseEntry *entries, int length) {
  AstNode n = {.type = NODE_CASE};
  n.asCase.subject = subject;
  n.asCase.entries = NULL;
  n.asCase.length = length;
  if (length > 0) {
    n.asCase.entries = gcmall(sizeof(struct AstCaseEntry) * length);
    memcpy(n.asCase.entries, entries, sizeof(struct AstCaseEntry) * length);
  }
  return createAst(root, n);
}

int createMemberAssign(Node *root, int target, int value) {
  if (!root || target < 0 || value < 0) return -1;
  AstNode node = {.type = NODE_MEMBER_ASSIGN, .memberAssign = {.target = target, .value = value}};
  return createAst(root, node);
}

int createStringInterp(Node *root, int *parts, int length) {
  AstNode n = {.type = NODE_STRING_INTERP};
  n.stringInterp.parts = copyIds(parts, length);
  n.stringInterp.length = length;
  return createAst(root, n);
}

int createModuleImport(Node *root, int basePath, struct AstModuleImportEntry *entries,
                       int entryCount, int alias) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MODULE_IMPORT};
  n.moduleImport.basePath = basePath;
  n.moduleImport.entries = NULL;
  n.moduleImport.entryCount = entryCount;
  n.moduleImport.alias = alias;
  if (entryCount > 0 && entries) {
    n.moduleImport.entries = gcmall(sizeof(struct AstModuleImportEntry) * entryCount);
    memcpy(n.moduleImport.entries, entries, sizeof(struct AstModuleImportEntry) * entryCount);
  }
  return createAst(root, n);
}

int createExportDecl(Node *root, int namespaceName, int sourcePath, int selectiveItems,
                     struct AstExportPolicyEntry *policies, int policyCount) {
  if (!root) return -1;
  AstNode n = {.type = NODE_EXPORT_DECL};
  n.astExport.namespaceName = namespaceName;
  n.astExport.sourcePath = sourcePath;
  n.astExport.selectiveItems = selectiveItems;
  n.astExport.policies = NULL;
  n.astExport.policyCount = policyCount;
  if (policyCount > 0 && policies) {
    n.astExport.policies = gcmall(sizeof(struct AstExportPolicyEntry) * policyCount);
    if (n.astExport.policies) {
      for (int i = 0; i < policyCount; i++) {
        n.astExport.policies[i].nameNode = policies[i].nameNode;
        n.astExport.policies[i].policy = policies[i].policy ? gcstrdup(policies[i].policy) : NULL;
      }
    }
  }
  return createAst(root, n);
}
