#include <rupa.h>

/*
 * Export grammar: handles export declarations with optional policy blocks.
 *
 *   export a, b from ./c
 *   export c from ./c
 *   export c from ./c -> { a: private }
 */

/* Parse policy entries inside `-> { name: private/public, ... }`. */
static int parseExportPolicies(Request *r, Token *t, int policyStart, int b,
                                struct AstExportPolicyEntry *policies) {
  int count = 0;
  if (policyStart < b && t->data[policyStart].type == ARROW) {
    policyStart++;
    if (policyStart < b && t->data[policyStart].type == LBRACE) {
      policyStart++;
      while (policyStart < b && t->data[policyStart].type != RBRACE) {
        if (t->data[policyStart].type == COMMA) {
          policyStart++;
          continue;
        }

        if (t->data[policyStart].type != IDENTIFIER &&
            t->data[policyStart].type != LITERAL_ID)
          break;

        int nameNode = createString(r->node, t->data[policyStart].value,
                                    NODE_LITERAL_ID);
        policyStart++;

        if (policyStart >= b || t->data[policyStart].type != COLON)
          break;
        policyStart++;

        if (policyStart >= b ||
            (t->data[policyStart].type != KEYWORD &&
             t->data[policyStart].type != IDENTIFIER))
          break;

        policies[count].nameNode = nameNode;
        policies[count].policy = strdup(t->data[policyStart].value);
        count++;
        policyStart++;
      }
      if (policyStart < b && t->data[policyStart].type == RBRACE)
        policyStart++;
    }
  }
  return count;
}

/* Handle export: `export a, b from ./c` or `export c from ./c -> {...}` */
int grammarParseExport(Request *r, Token *t, int a, int b, int *pos) {
  int cur = a + 1;

  /* Collect names before 'from' */
  int names[64];
  int nameCount = 0;
  int fromPos = -1;

  while (cur < b) {
    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "from")) {
      fromPos = cur;
      break;
    }

    if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID)
      break;

    names[nameCount++] = cur;
    cur++;

    if (cur < b && t->data[cur].type == COMMA)
      cur++;
  }

  if (nameCount == 0 || fromPos < 0) {
    /* No 'from' found, fallback to old-style export */
    int v = grammarParseExpr(r, a + 1, b);
    *pos = b;
    return createModule(r->node, NODE_EXPORT, v, -1);
  }

  /* Parse 'from PATH' */
  int fromPathEnd = -1;
  int fromStart = grammarModuleDetectFrom(t, fromPos, b, &fromPathEnd);
  if (fromStart < 0) {
    *pos = b;
    return GRAMMAR_NO_MATCH;
  }

  char *fromPath = grammarModuleBuildPath(t, fromStart, fromPathEnd);
  int sourcePath = fromPath ? createString(r->node, fromPath, NODE_LITERAL_ID)
                            : -1;
  free(fromPath);

  /* Parse optional '-> { ... }' policy block */
  struct AstExportPolicyEntry policies[64];
  int policyCount =
      parseExportPolicies(r, t, fromPathEnd + 1, b, policies);

  /* Determine export type */
  int namespaceName = -1;
  int selectiveItems = -1;

  if (nameCount == 1) {
    namespaceName = createString(r->node, t->data[names[0]].value,
                                 NODE_LITERAL_ID);
  } else {
    int nameIds[64];
    for (int i = 0; i < nameCount; i++)
      nameIds[i] = createString(r->node, t->data[names[i]].value,
                                NODE_LITERAL_ID);
    selectiveItems = createArray(r->node, nameIds, nameCount);
  }

  int policyEnd = (policyCount > 0) ? fromPathEnd + 2 : fromPathEnd + 1;
  /* Advance past policy block if present */
  if (policyCount > 0) {
    int scan = fromPathEnd + 1;
    if (scan < b && t->data[scan].type == ARROW) {
      scan++;
      if (scan < b && t->data[scan].type == LBRACE) {
        scan++;
        while (scan < b && t->data[scan].type != RBRACE)
          scan++;
        if (scan < b)
          scan++;
      }
      policyEnd = scan;
    }
  }

  *pos = policyEnd;
  return createExportDecl(r->node, namespaceName, sourcePath, selectiveItems,
                          policyCount > 0 ? policies : NULL, policyCount);
}
