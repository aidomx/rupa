#include <rupa.h>

/* ---- Module design baru (NODE_MOD) — 1 container untuk import & export ----
 * Semua alokasi via GC (gccalloc/gcdup), tidak ada free manual.
 */

AstModEntry *modEntry(const char *name) {
  AstModEntry *e = gccalloc(1, sizeof(AstModEntry));
  if (!e) return NULL;
  e->type = MOD_ID;
  e->name = gcdup(name);
  return e;
}

AstModEntry *modEntryKey(const char *name, const char *key) {
  AstModEntry *e = modEntry(name);
  if (!e) return NULL;
  e->key = gcdup(key);
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
  e->value = gcdup(value);
  return e;
}

/* Copy array of entry pointers ke GC — struktur rantai childrens ikut diduplikat. */
static AstModEntry *modCopyEntries(AstModEntry **entries, int entryCount) {
  if (!entries || entryCount <= 0) return NULL;
  AstModEntry *out = gccalloc(entryCount, sizeof(AstModEntry));
  if (!out) return NULL;
  for (int i = 0; i < entryCount; i++) {
    AstModEntry *src = entries[i];
    if (!src) continue;
    out[i].type = src->type;
    out[i].name = src->name ? gcdup(src->name) : NULL;
    out[i].key = src->key ? gcdup(src->key) : NULL;
    out[i].value = src->value ? gcdup(src->value) : NULL;
    /* Rantai childrens disalin (shallow per node, GC-owned). */
    AstModEntry *s = src->childrens;
    AstModEntry *dst = NULL;
    while (s) {
      AstModEntry *c = gccalloc(1, sizeof(AstModEntry));
      if (!c) break;
      c->type = s->type;
      c->name = s->name ? gcdup(s->name) : NULL;
      c->key = s->key ? gcdup(s->key) : NULL;
      c->value = s->value ? gcdup(s->value) : NULL;
      if (dst)
        dst->childrens = c;
      else
        out[i].childrens = c;
      dst = c;
      s = s->childrens;
    }
  }
  return out;
}

int createModImport(Node *root, AstModEntry **entries, int entryCount, const char *source,
                    const char *sourceAlias) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MOD};
  n.mod.type = ImportDecl;
  n.mod.entries = modCopyEntries(entries, entryCount);
  n.mod.entryCount = entryCount;
  n.mod.source = source ? gcdup(source) : NULL;
  n.mod.sourceAlias = sourceAlias ? gcdup(sourceAlias) : NULL;
  n.mod.policies = NULL;
  n.mod.policyCount = 0;
  n.mod.body = -1;
  return createAst(root, n);
}

int createModExport(Node *root, AstModEntry **entries, int entryCount, const char *source,
                    AstModEntry **policies, int policyCount) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MOD};
  n.mod.type = ExportDecl;
  n.mod.entries = modCopyEntries(entries, entryCount);
  n.mod.entryCount = entryCount;
  n.mod.source = source ? gcdup(source) : NULL;
  n.mod.sourceAlias = NULL;
  n.mod.policies = modCopyEntries(policies, policyCount);
  n.mod.policyCount = policies ? policyCount : 0;
  n.mod.body = -1;
  return createAst(root, n);
}

/* `namespace db { export ...; export ...; }` — body is a NODE_BLOCK id
 * holding the nested export statements; name is reused via mod.source
 * (the field already means "identity of what's being bound" for exports). */
int createModNamespace(Node *root, const char *name, int body) {
  if (!root) return -1;
  AstNode n = {.type = NODE_MOD};
  n.mod.type = NamespaceDecl;
  n.mod.entries = NULL;
  n.mod.entryCount = 0;
  n.mod.source = name ? gcdup(name) : NULL;
  n.mod.sourceAlias = NULL;
  n.mod.policies = NULL;
  n.mod.policyCount = 0;
  n.mod.body = body;
  return createAst(root, n);
}

/* AST Node Factory — Statements, Module, Async, Case
 * Core factory functions remain in factory.c.
 * createAst and copyIds are declared in rupa_modules.h
 */
int createComment(Node *root, const char *value, int type) {
  if (!root) return -1;
  AstNode n = {.type = NODE_COMMENT};
  n.asComment.type = type;
  n.asComment.value = gcdup(value);
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

int createIf(Node *root, int condition, int thenBlock, int elseBlock, bool isBlock) {
  AstNode n = {.type = NODE_IF};
  n.asIf.condition = condition;
  n.asIf.thenBlock = thenBlock;
  n.asIf.elseBlock = elseBlock;
  n.asIf.isBlock = isBlock;
  return createAst(root, n);
}

int createLoop(Node *root, const char *kind, int condition, int body) {
  AstNode n = {.type = NODE_LOOP};
  n.loop.kind = gcdup(kind ? kind : "");
  n.loop.condition = condition;
  n.loop.body = body;
  return createAst(root, n);
}

int createFunctionDecl(Node *root, int name, int *params, int paramLength, int body,
                       int returnType) {
  AstNode n = {.type = NODE_FUNCTION_DECL};
  n.function.name = name;
  n.function.params = copyIds(params, paramLength);
  n.function.paramLength = paramLength;
  n.function.body = body;
  n.function.returnType = returnType;
  return createAst(root, n);
}

int createStructDecl(Node *root, int name, int body) {
  AstNode n = {.type = NODE_STRUCT_DECL};
  n.asStruct.name = name;
  n.asStruct.body = body;
  return createAst(root, n);
}

int createClassDecl(Node *root, int name, int typeId, int body) {
  AstNode n = {.type = NODE_CLASS_DECL};
  n.asClass.name = name;
  n.asClass.type = typeId;
  n.asClass.body = body;
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
