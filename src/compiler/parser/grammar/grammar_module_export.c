#include <rupa.h>

/*
 * Export grammar — emits NODE_MOD (AstMod, ExportDecl) for all forms:
 *
 *   export a, b from ./c                 (selective re-export)
 *   export c from ./c                    (namespace re-export, any alias)
 *   export c from ./c -> { a: private }  (namespace with policies)
 *   export x                             (local value; source = NULL)
 *   export x as y                        (local value, renamed on publish)
 *   export dbtest -> { getUser }         (local with policies)
 *   export driver.connect as driverConnect
 *                                        (dotted path, no `from` needed —
 *                                         first segment is an implicit file
 *                                         relative to the current directory;
 *                                         same convention as import's `a.create`)
 *   export driver.* as db                (whole implicit-file re-export, aliased)
 *
 * Plus the block form, which merges several of the above under one name:
 *
 *   namespace db {
 *     export driver
 *     export login, register from table
 *     export table -> { rawQuery: private }
 *   }
 *
 * See grammar_module_import.c for `modEntryFromPath` (shared: dotted-path +
 * wildcard parsing is identical on both sides of the language).
 */

/* Parse policy entries inside `-> { name: private/public, ... }`.
 * Values other than private/public are still captured as-is. */
static int parseExportPolicies(Token *t, int policyStart, int b, AstModEntry **policies) {
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

        if (t->data[policyStart].type != IDENTIFIER && t->data[policyStart].type != LITERAL_ID)
          break;

        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s", t->data[policyStart].value);
        policyStart++;

        if (policyStart >= b || t->data[policyStart].type != COLON)
          break;
        policyStart++;

        if (policyStart >= b ||
            (t->data[policyStart].type != KEYWORD && t->data[policyStart].type != IDENTIFIER))
          break;

        policies[count] = modPolicy(nameBuf, t->data[policyStart].value);
        count++;
        policyStart++;
      }
      if (policyStart < b && t->data[policyStart].type == RBRACE)
        policyStart++;
    }
  }
  return count;
}

/* Parse export entries between baseIdx and limit: dotted paths
 * (`driver.connect`), wildcards (`driver.*`), and `as alias`. Mirrors
 * parseFlatImportEntries in grammar_module_import.c so both sides of the
 * language treat `x.y.z [as w]` identically. Stops at `from` or `->`,
 * leaving those to the caller. */
static int parseExportEntries(Token *t, int baseIdx, int limit, AstModEntry **entries,
                              int *entryCount) {
  int cur = baseIdx;
  *entryCount = 0;

  while (cur < limit) {
    if (t->data[cur].type == COMMA) {
      cur++;
      continue;
    }

    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "from"))
      break;
    if (t->data[cur].type == ARROW) break;

    if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID) break;

    int pathStart = cur;
    cur++;
    while (cur < limit && t->data[cur].type == DOT) {
      cur++;
      if (cur >= limit || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
        break;
      cur++;
    }

    char pathBuf[256] = {0};
    for (int i = pathStart; i < cur; i++) {
      if (t->data[i].type == DOT) continue;
      if (pathBuf[0]) strcat(pathBuf, ".");
      strcat(pathBuf, t->data[i].value);
    }

    /* Wildcard: path.* */
    bool wild = false;
    if (cur < limit && t->data[cur].type == STAR) {
      wild = true;
      cur++;
    }

    AstModEntry *e = modEntryFromPath(pathBuf, wild);
    if (!e) break;
    entries[(*entryCount)++] = e;

    /* Optional `as alias` */
    if (cur < limit &&
        (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "as")) {
      cur++;
      if (cur < limit && (t->data[cur].type == IDENTIFIER || t->data[cur].type == LITERAL_ID)) {
        e->key = gcstrdup(t->data[cur].value);
        cur++;
      }
      /* Trailing wildcard after alias: `driver as db.*` → wildcard entry */
      if (cur + 1 < limit && t->data[cur].type == DOT && t->data[cur + 1].type == STAR) {
        e->type = MOD_WILD;
        cur += 2;
      }
    }
  }
  return cur;
}

/* Handle export: `export a, b from ./c`, `export c from ./c -> {...}`,
 * local `export x [as y]`, or dotted local `export driver.connect as w`
 * (no `from` — first segment resolves to an implicit file). */
int grammarParseExport(Request *r, Token *t, int a, int b, int *pos) {
  int cur = a + 1;

  AstModEntry *entries[64];
  int entryCount = 0;
  cur = parseExportEntries(t, cur, b, entries, &entryCount);

  int fromPos = -1;
  if (cur < b &&
      (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
       t->data[cur].type == LITERAL_ID) &&
      !strcmp(t->data[cur].value, "from"))
    fromPos = cur;

  if (entryCount == 0 || fromPos < 0) {
    /* No 'from' → local export (`export x`, `export x as y`, or dotted
     * `export driver.connect as w` — entries already carry alias/wild/
     * member-chain info from parseExportEntries). Still allow an optional
     * `-> {...}` policy block, e.g. `export dbtest -> { getUser: private }`. */
    AstModEntry *policies[64];
    int policyCount = parseExportPolicies(t, cur, b, policies);

    int end = cur;
    if (policyCount > 0) {
      int scan = cur;
      if (scan < b && t->data[scan].type == ARROW) {
        scan++;
        if (scan < b && t->data[scan].type == LBRACE) {
          scan++;
          while (scan < b && t->data[scan].type != RBRACE) scan++;
          if (scan < b) scan++;
        }
        end = scan;
      }
    }

    *pos = (end > cur) ? end : b;
    return createModExport(r->node, entries, entryCount, NULL, policies, policyCount);
  }

  /* Parse 'from PATH' */
  int fromPathEnd = -1;
  int fromStart = grammarModuleDetectFrom(t, fromPos, b, &fromPathEnd);
  if (fromStart < 0) {
    *pos = b;
    return GRAMMAR_NO_MATCH;
  }

  char *fromPath = grammarModuleBuildPath(t, fromStart, fromPathEnd);

  /* Parse optional '-> { ... }' policy block */
  AstModEntry *policies[64];
  int policyCount = parseExportPolicies(t, fromPathEnd + 1, b, policies);

  /* Compute end position: past policy block if present */
  int policyEnd = fromPathEnd + 1;
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
  int id = createModExport(r->node, entries, entryCount, fromPath, policies, policyCount);
  free(fromPath);
  return id;
}

/* Handle `namespace name { export ...; export ...; }`.
 * The body's statements are parsed by the ordinary statement grammar
 * (grammarParseKeywordBody → grammarParseBlock → grammarParseStatement) —
 * each line inside is just a normal `export ...` statement. The interpreter
 * (dispatch.c) walks the resulting NODE_BLOCK, evaluates each ExportDecl,
 * and merges the results under `name` instead of binding them to env
 * directly, erroring on duplicate bind-names within the block. */
int grammarParseNamespace(Request *r, Token *t, int a, int limit, int *pos) {
  int cur = a + 1;
  if (cur >= limit || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
    return GRAMMAR_NO_MATCH;

  char nameBuf[128];
  snprintf(nameBuf, sizeof(nameBuf), "%s", t->data[cur].value);
  cur++;

  while (cur < limit && grammarIsWhitespace(t, cur)) cur++;
  if (cur >= limit || t->data[cur].type != LBRACE) return GRAMMAR_NO_MATCH;

  int next = cur;
  int body = grammarParseKeywordBody(r, cur, limit, &next);
  if (body < 0) return GRAMMAR_NO_MATCH;

  *pos = next;
  return createModNamespace(r->node, nameBuf, body);
}
