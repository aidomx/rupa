#include <rupa.h>

bool printControlAst(Node *node, int index, int level) {
  if (!node || index < 0 || index >= node->length)
    return false;

  AstNode *n = &node->ast[index];
  switch (n->type) {
  case NODE_BREAK:
    printIndent(level);
    printf("Break\n");
    return true;
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
    if (n->update.target >= 0)
      printAst(node, n->update.target, level + 1);
    return true;
  case NODE_LOOP:
    printIndent(level);
    printf("Loop: %s\n", n->loop.kind);
    if (n->loop.condition >= 0)
      printAst(node, n->loop.condition, level + 1);
    if (n->loop.body >= 0)
      printAst(node, n->loop.body, level + 1);
    return true;
  case NODE_RETURN:
    printIndent(level);
    printf("Return:\n");
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
  case NODE_IMPORT:
  case NODE_EXPORT:
  case NODE_EXTENDS:
    printIndent(level);
    printf("Module statement:\n");
    if (n->module.value >= 0)
      printAst(node, n->module.value, level + 1);
    if (n->module.name >= 0) {
      printIndent(level + 1);
      printf("from: ");
      printAst(node, n->module.name, level + 1);
    }
    return true;
  case NODE_EXPORT_DECL:
    printIndent(level);
    printf("Export Declaration:\n");
    if (n->astExport.namespaceName >= 0) {
      printIndent(level + 1);
      printf("namespace: ");
      printAst(node, n->astExport.namespaceName, level + 2);
    }
    if (n->astExport.selectiveItems >= 0) {
      printIndent(level + 1);
      printf("selective items: ");
      printAst(node, n->astExport.selectiveItems, level + 2);
    }
    if (n->astExport.sourcePath >= 0) {
      printIndent(level + 1);
      printf("from: ");
      printAst(node, n->astExport.sourcePath, level + 2);
    }
    if (n->astExport.policyCount > 0 && n->astExport.policies) {
      printIndent(level + 1);
      printf("policies:\n");
      for (int i = 0; i < n->astExport.policyCount; i++) {
        printIndent(level + 2);
        printf("%s: %s\n",
               n->astExport.policies[i].nameNode >= 0
                   ? (node->ast[n->astExport.policies[i].nameNode].type == NODE_LITERAL_ID
                          ? node->ast[n->astExport.policies[i].nameNode].string.value
                          : "?")
                   : "?",
               n->astExport.policies[i].policy
                   ? n->astExport.policies[i].policy
                   : "?");
      }
    }
    return true;
  case NODE_MODULE_IMPORT:
    printIndent(level);
    printf("Module Import:\n");
    printIndent(level + 1);
    printf("base: ");
    if (n->moduleImport.basePath >= 0)
      printAst(node, n->moduleImport.basePath, level + 2);
    else
      printf("(null)");
    for (int i = 0; i < n->moduleImport.entryCount; i++) {
      struct AstModuleImportEntry *e = &n->moduleImport.entries[i];
      printIndent(level + 1);
      printf("entry: ");
      if (e->pathNode >= 0)
        printAst(node, e->pathNode, level + 2);
      if (e->aliasNode >= 0) {
        printf(" as ");
        printAst(node, e->aliasNode, level + 2);
      }
      if (e->isWildcard)
        printf(" (wildcard)");
      printf("\n");
    }
    if (n->moduleImport.alias >= 0) {
      printIndent(level + 1);
      printf("alias: ");
      printAst(node, n->moduleImport.alias, level + 2);
    }
    return true;
  default:
    return false;
  }
}
