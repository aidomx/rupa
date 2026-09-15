#include <rupa.h>

bool printControlAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length) return false;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_BREAK:
    printIndent(level);
    printf("Break\n");
    return true;
  case NODE_COMMENT:
  case NODE_INLINE_COMMENT:
  case NODE_BLOCK_COMMENT: {
    printIndent(level);
    printf("Comment:\n");
    printIndent(level + 1);
    printf("Type:\n");
    /* n->asComment.type adalah enum kind, BUKAN node id — jangan
     * diteruskan ke printAst (dulu menyebabkan rekursi tak berujung
     * saat pool AST cukup besar sehingga id enum valid). */
    const char *kind = (n->asComment.type == NODE_BLOCK_COMMENT) ? "BlockComment" : "InlineComment";
    printIndent(level + 2);
    printf("%s\n", kind);
    printIndent(level + 1);
    printf("Value:\n");
    printIndent(level + 3);
    char str[MAX_BUFFER_SIZE];
    serialize(n->asComment.value, str);
    printf("%s\n", str);
    return true;
  }

  case NODE_CONTINUE:
    printIndent(level);
    printf("Continue\n");
    return true;
  case NODE_CONDITIONAL_ASSIGN:
    printIndent(level);
    printf("Conditional Assignment:\n");
    printIndent(level + 1);
    printf("Target:\n");
    printAst(node, n->conditionalAssign.target, level + 2);
    printIndent(level + 1);
    printf("Value:\n");
    printAst(node, n->conditionalAssign.value, level + 2);
    return true;
  case NODE_THEN:
    printIndent(level);
    printf("Then:\n");
    printIndent(level + 1);
    printf("Condition:\n");
    printAst(node, n->then.condition, level + 2);
    printIndent(level + 1);
    printf("Result:\n");
    printAst(node, n->then.result, level + 2);
    return true;
  case NODE_FALLBACK:
    printIndent(level);
    printf("Fallback:\n");
    printIndent(level + 1);
    printf("Primary:\n");
    printAst(node, n->fallback.primary, level + 2);
    printIndent(level + 1);
    printf("Fallback:\n");
    printAst(node, n->fallback.fallback, level + 2);
    return true;
  case NODE_UPDATE:
    printIndent(level);
    printf("Update: %s%s\n", n->update.prefix ? "prefix " : "postfix ",
           n->update.op ? n->update.op : "?");
    if (n->update.target >= 0) printAst(node, n->update.target, level + 1);
    if (n->update.value >= 0) printAst(node, n->update.value, level + 1);
    return true;
  case NODE_LOOP:
    printIndent(level);
    printf("Loop: %s\n", n->loop.kind);
    if (n->loop.condition >= 0) printAst(node, n->loop.condition, level + 1);
    if (n->loop.body >= 0) printAst(node, n->loop.body, level + 1);
    return true;
  case NODE_RETURN:
    printIndent(level);
    printf(n->asReturn.explicitReturn ? "Return:\n" : "Expression statement:\n");
    printAst(node, n->asReturn.expression, level + 1);
    return true;
  case NODE_CASE:
    printIndent(level);
    printf("Case:\n");
    printIndent(level + 1);
    printf("Subject:\n");
    printAst(node, n->asCase.subject, level + 2);
    for (int i = 0; i < n->asCase.length; i++) {
      struct AstCaseEntry *e = &n->asCase.entries[i];
      printIndent(level + 1);
      printf(e->wildcard ? "Wildcard:\n" : "Entry:\n");
      if (!e->wildcard) {
        printIndent(level + 2);
        printf("Pattern:\n");
        printAst(node, e->pattern, level + 3);
      }
      printIndent(level + 2);
      printf("Body:\n");
      printAst(node, e->body, level + 3);
    }
    return true;
  case NODE_ASYNC:
    printIndent(level);
    printf("Async:\n");
    printIndent(level + 1);
    printf("Request:\n");
    printAst(node, n->async.request, level + 2);
    if (n->async.handler >= 0) {
      printIndent(level + 1);
      printf("Handler:\n");
      printAst(node, n->async.handler, level + 2);
    }
    if (n->async.loaderId >= 0) {
      printIndent(level + 1);
      printf("Loader:\n");
      printAst(node, n->async.loaderId, level + 2);
    }
    if (n->async.timeoutId >= 0) {
      printIndent(level + 1);
      printf("TimeoutId:\n");
      printAst(node, n->async.timeoutId, level + 2);
    }
    if (n->async.timeout >= 0) {
      printIndent(level + 1);
      printf("Timeout:\n");
      printAst(node, n->async.timeout, level + 2);
    }
    return true;
  case NODE_AWAIT:
    printIndent(level);
    printf("Await:\n");
    printAst(node, n->await.expression, level + 1);
    return true;
  case NODE_STRING_INTERP:
    printIndent(level);
    printf("StringInterp:\n");
    for (int i = 0; i < n->stringInterp.length; i++)
      printAst(node, n->stringInterp.parts[i], level + 1);
    return true;
  case NODE_MOD: {
    printIndent(level);

    if (n->mod.type == NamespaceDecl) {
      printf("Namespace Declaration:\n");
      if (n->mod.source) {
        printIndent(level + 1);
        printf("Name: %s\n", n->mod.source);
      }
      if (n->mod.body >= 0) {
        printIndent(level + 1);
        printf("Body:\n");
        printAst(node, n->mod.body, level + 2);
      }
      return true;
    }

    printf("%s Declaration:\n", n->mod.type == ImportDecl ? "Import" : "Export");
    printIndent(level + 1);
    printf("entries:\n");
    for (int i = 0; i < n->mod.entryCount; i++) {
      struct AstModEntry *e = &n->mod.entries[i];
      printIndent(level + 2);
      printf("%s", e->name ? e->name : "?");
      if (e->type == MOD_WILD)
        printf(".*");
      for (struct AstModEntry *c = e->childrens; c; c = c->childrens)
        printf(".%s", c->name ? c->name : "?");
      if (e->key)
        printf(" as %s", e->key);
      printf("\n");
    }
    if (n->mod.source) {
      printIndent(level + 1);
      printf("from: %s", n->mod.source);
      if (n->mod.sourceAlias)
        printf(" as %s", n->mod.sourceAlias);
      printf("\n");
    }
    if (n->mod.policyCount > 0 && n->mod.policies) {
      printIndent(level + 1);
      printf("policies:\n");
      for (int i = 0; i < n->mod.policyCount; i++) {
        printIndent(level + 2);
        printf("%s: %s\n", n->mod.policies[i].name ? n->mod.policies[i].name : "?",
               n->mod.policies[i].value ? n->mod.policies[i].value : "?");
      }
    }
    return true;
  }
  case NODE_EXTENDS:
    printIndent(level);
    printf("Module Declaration:\n");
    if (n->module.value >= 0) printAst(node, n->module.value, level + 1);
    if (n->module.name >= 0) {
      printIndent(level + 1);
      printf("from: ");
      printAst(node, n->module.name, level + 1);
    }
    return true;
  default:
    return false;
  }
}
