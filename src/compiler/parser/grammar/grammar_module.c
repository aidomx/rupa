#include <rupa.h>

/*
 * Detect if tokens from `from_pos` onward match: from rupa.MODULE
 * Returns module name token index on success, -1 on failure.
 */
static int detectFromRupa(Token *t, int from_pos, int limit) {
  if ((from_pos + 2) >= limit)
    return -1;
  if ((t->data[from_pos].type != IDENTIFIER &&
       t->data[from_pos].type != KEYWORD) ||
      strcmp(t->data[from_pos].value, "from"))
    return -1;
  if ((t->data[from_pos + 1].type != IDENTIFIER &&
       t->data[from_pos + 1].type != LITERAL_ID) ||
      strcmp(t->data[from_pos + 1].value, "rupa"))
    return -1;
  if (t->data[from_pos + 2].type != DOT)
    return -1;
  int mod = from_pos + 3;
  if (mod >= limit)
    return -1;
  if (t->data[mod].type != IDENTIFIER && t->data[mod].type != LITERAL_ID)
    return -1;
  return mod;
}

/*
 * Detect general `from X.Y.Z` pattern (any module, not just rupa).
 * Returns start index of the first identifier after `from`, or -1.
 * Also sets *end to the index of the last token in the path.
 */
static int detectFrom(Token *t, int from_pos, int limit, int *end) {
  if ((from_pos + 2) >= limit)
    return -1;
  if ((t->data[from_pos].type != IDENTIFIER &&
       t->data[from_pos].type != KEYWORD &&
       t->data[from_pos].type != LITERAL_ID) ||
      strcmp(t->data[from_pos].value, "from"))
    return -1;
  int mod_start = from_pos + 1;
  if (t->data[mod_start].type != IDENTIFIER &&
      t->data[mod_start].type != LITERAL_ID)
    return -1;
  /* Scan for DOT.IDENTIFIER chains */
  int mod_end = mod_start;
  int cur = mod_start + 1;
  while (cur + 1 < limit && t->data[cur].type == DOT) {
    if (t->data[cur + 1].type != IDENTIFIER &&
        t->data[cur + 1].type != LITERAL_ID)
      break;
    mod_end = cur + 1;
    cur += 2;
  }
  *end = mod_end;
  return mod_start;
}

/*
 * Build a concatenated module path string from tokens[start..end],
 * joining with dots. E.g. tokens `modules` `.` `test_1` -> "modules.test_1".
 * Caller must free() the result.
 */
static char *buildModulePath(Token *t, int start, int end) {
  /* Calculate total length */
  int total = 0;
  for (int i = start; i <= end; i++) {
    if (t->data[i].type != DOT)
      total += (int)strlen(t->data[i].value);
  }
  /* Add dots between parts */
  int parts = 0;
  for (int i = start; i <= end; i++)
    if (t->data[i].type != DOT) parts++;
  total += (parts - 1); /* dots between parts */

  char *buf = malloc(total + 1);
  if (!buf) return NULL;
  buf[0] = '\0';
  int first = 1;
  for (int i = start; i <= end; i++) {
    if (t->data[i].type == DOT) continue;
    if (!first) strcat(buf, ".");
    strcat(buf, t->data[i].value);
    first = 0;
  }
  return buf;
}

/*
 * Detect the new flat import syntax:
 *   import a.create, b.login as auth, d.* from modules as m
 *
 * Returns the index of the first dot-separated name token, or -1.
 * Sets *fromPos to the index of the `from` keyword.
 */static int detectFlatImport(Token *t, int a, int b, int *fromPos) {
  /* Need at least: IDENTIFIER.IDENTIFIER from IDENTIFIER */
  if (a + 2 >= b)
    return -1;

  /* First token must be identifier (start of path.name) */
  if (t->data[a].type != IDENTIFIER && t->data[a].type != LITERAL_ID)
    return -1;

  /* The first entry must contain a dot (a.create style), otherwise it's old syntax */
  if (a + 1 >= b || t->data[a + 1].type != DOT)
    return -1;

  /* Scan forward to find `from` keyword */
  int cur = a;
  while (cur < b) {
    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "from")) {
      *fromPos = cur;
      return a;
    }
    /* Skip commas and identifiers */
    if (t->data[cur].type == COMMA || t->data[cur].type == IDENTIFIER ||
        t->data[cur].type == LITERAL_ID || t->data[cur].type == DOT ||
        t->data[cur].type == KEYWORD || !strcmp(t->data[cur].value, "as") ||
        t->data[cur].type == STAR) {
      cur++;
      continue;
    }
    break;
  }
  return -1;
}

int grammarParseModule(Request *r, int a, int b, int *pos) {
  Token *t = r->tokens;
  const char *k = t->data[a].value;

  if (strcmp(k, "import") && strcmp(k, "export") && strcmp(k, "extends"))
    return GRAMMAR_NO_MATCH;

  NodeType nt = !strcmp(k, "import")
                    ? NODE_IMPORT
                    : (!strcmp(k, "export") ? NODE_EXPORT : NODE_EXTENDS);

  /* --- NEW: import a.create, b.login as auth, d.* from modules as m --- */
  if (nt == NODE_IMPORT) {
    int fromPos = -1;
    int baseIdx = detectFlatImport(t, a + 1, b, &fromPos);
    if (baseIdx >= 0 && fromPos >= 0) {
      /* Parse import entries */
      struct AstModuleImportEntry entries[64];
      int entryCount = 0;
      int cur = baseIdx;

      while (cur < fromPos) {
        /* Skip commas */
        if (t->data[cur].type == COMMA) {
          cur++;
          continue;
        }

        /* Parse path: IDENTIFIER ('.' IDENTIFIER)* [".*" | [" as alias"]] */
        if (cur >= fromPos || (t->data[cur].type != IDENTIFIER &&
                               t->data[cur].type != LITERAL_ID))
          break;

        int pathStart = cur;
        cur++;
        while (cur < fromPos && t->data[cur].type == DOT) {
          cur++;
          if (cur >= fromPos || (t->data[cur].type != IDENTIFIER &&
                                 t->data[cur].type != LITERAL_ID))
            break;
          cur++;
        }

        /* Check for wildcard: path.* */
        if (cur < fromPos && t->data[cur].type == STAR) {
          char pathBuf[256] = {0};
          for (int i = pathStart; i < cur; i++) {
            if (t->data[i].type == DOT) continue;
            if (pathBuf[0]) strcat(pathBuf, ".");
            strcat(pathBuf, t->data[i].value);
          }
          entries[entryCount].pathNode =
              createString(r->node, pathBuf, NODE_LITERAL_ID);
          entries[entryCount].aliasNode = -1;
          entries[entryCount].isWildcard = true;
          entryCount++;
          cur++;
          continue;
        }

        /* Regular entry: path parsed by while loop, check for optional as alias */
        {
          char pathBuf[256] = {0};
          for (int i = pathStart; i < cur; i++) {
            if (t->data[i].type == DOT) continue;
            if (pathBuf[0]) strcat(pathBuf, ".");
            strcat(pathBuf, t->data[i].value);
          }

          entries[entryCount].pathNode =
              createString(r->node, pathBuf, NODE_LITERAL_ID);
          entries[entryCount].isWildcard = false;
          entries[entryCount].aliasNode = -1;

          /* Check for `as alias` */
          int nextCur = cur;
          if (nextCur < fromPos &&
              (t->data[nextCur].type == KEYWORD ||
               t->data[nextCur].type == IDENTIFIER ||
               t->data[nextCur].type == LITERAL_ID) &&
              !strcmp(t->data[nextCur].value, "as")) {
            nextCur++;
            if (nextCur < fromPos &&
                (t->data[nextCur].type == IDENTIFIER ||
                 t->data[nextCur].type == LITERAL_ID)) {
              entries[entryCount].aliasNode =
                  createString(r->node, t->data[nextCur].value,
                               NODE_LITERAL_ID);
              nextCur++;
            }
          }

          entryCount++;
          cur = nextCur;
          continue;
        }
      }

      if (entryCount == 0) {
        *pos = b;
        return GRAMMAR_NO_MATCH;
      }

      /* Parse `from PATH` */
      int fromPathEnd = -1;
      int fromStart = detectFrom(t, fromPos, b, &fromPathEnd);
      if (fromStart < 0) {
        *pos = b;
        return GRAMMAR_NO_MATCH;
      }

      char *fromPath = buildModulePath(t, fromStart, fromPathEnd);
      int basePath = fromPath ? createString(r->node, fromPath, NODE_LITERAL_ID)
                              : -1;
      free(fromPath);

      /* Check for optional `as alias` after from path */
      int aliasIdx = -1;
      int afterPath = fromPathEnd + 1;
      if (afterPath < b &&
          (t->data[afterPath].type == KEYWORD ||
           t->data[afterPath].type == IDENTIFIER ||
           t->data[afterPath].type == LITERAL_ID) &&
          !strcmp(t->data[afterPath].value, "as")) {
        afterPath++;
        if (afterPath < b && (t->data[afterPath].type == IDENTIFIER ||
                              t->data[afterPath].type == LITERAL_ID)) {
          aliasIdx = createString(r->node, t->data[afterPath].value,
                                  NODE_LITERAL_ID);
          afterPath++;
        }
      }

      *pos = afterPath;
      return createModuleImport(r->node, basePath, entries, entryCount, aliasIdx);
    }
  }

  /* --- OLD: import X, Y, ... from rupa.MODULE --- */
  if (nt == NODE_IMPORT) {
    /* Scan for comma-separated identifiers starting at a+1.
     * Count how many names there are before hitting `from`. */
    int names[64];
    int nameCount = 0;
    int cur = a + 1;

    while (cur < b) {
      /* Check if this token starts "from rupa.M" (stdlib) */
      int modIdx = detectFromRupa(t, cur, b);
      if (modIdx >= 0) {
        /* Found the `from rupa.M` clause */
        if (nameCount == 0)
          break;
        if (nameCount == 1) {
          /* Single import: import X from rupa.Y -> value=X, name=Y */
          int v = createString(r->node, t->data[names[0]].value,
                               NODE_LITERAL_ID);
          int n = createString(r->node, t->data[modIdx].value,
                               NODE_LITERAL_ID);
          *pos = b;
          return createModule(r->node, nt, v, n);
        }
        /* Multiple imports: import X, Z from rupa.Y -> value=ARRAY, name=Y */
        int nameIds[64];
        for (int i = 0; i < nameCount; i++)
          nameIds[i] = createString(r->node, t->data[names[i]].value,
                                    NODE_LITERAL_ID);
        int v = createArray(r->node, nameIds, nameCount);
        int n = createString(r->node, t->data[modIdx].value, NODE_LITERAL_ID);
        *pos = b;
        return createModule(r->node, nt, v, n);
      }

      /* Check if this token starts a general `from X.Y.Z` (local file import) */
      {
        int pathEnd = -1;
        int pathStart = detectFrom(t, cur, b, &pathEnd);
        if (pathStart >= 0 && pathEnd >= 0) {
          if (nameCount == 0)
            break;
          char *fullPath = buildModulePath(t, pathStart, pathEnd);
          if (nameCount == 1) {
            int v = createString(r->node, t->data[names[0]].value,
                                 NODE_LITERAL_ID);
            int n = fullPath ? createString(r->node, fullPath, NODE_LITERAL_ID)
                              : -1;
            free(fullPath);
            *pos = b;
            return createModule(r->node, nt, v, n);
          }
          /* Multiple imports: import X, Z from path -> value=ARRAY, name=path */
          int nameIds[64];
          for (int i = 0; i < nameCount; i++)
            nameIds[i] = createString(r->node, t->data[names[i]].value,
                                      NODE_LITERAL_ID);
          int v = createArray(r->node, nameIds, nameCount);
          int n = fullPath ? createString(r->node, fullPath, NODE_LITERAL_ID)
                            : -1;
          free(fullPath);
          *pos = b;
          return createModule(r->node, nt, v, n);
        }
      }

      /* Expect IDENTIFIER or LITERAL_ID */
      if (t->data[cur].type != IDENTIFIER &&
          t->data[cur].type != LITERAL_ID)
        break;

      names[nameCount++] = cur;
      cur++;

      /* Skip comma if present, then loop back to check detectFromRupa */
      if (cur < b && t->data[cur].type == COMMA)
        cur++;
      continue;
    }
  }

  /* Fallback: old-style import */
  int v = grammarParseExpr(r, a + 1, b);
  *pos = b;
  return createModule(r->node, nt, v, -1);
}
