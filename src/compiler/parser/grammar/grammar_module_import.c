#include <rupa.h>

/*
 * Import grammar: handles both the new flat syntax and the old-style syntax.
 *
 *   Flat:     import a.create, b.login as auth, d.* from modules as m
 *   Old:      import X, Y from rupa.MODULE
 *             import X, Y from path.to.file
 */

/* Parse flat import entries between baseIdx and fromPos. */
static int parseFlatImportEntries(Request *r, Token *t, int baseIdx,
                                  int fromPos,
                                  struct AstModuleImportEntry *entries,
                                  int *entryCount) {
  int cur = baseIdx;
  *entryCount = 0;

  while (cur < fromPos) {
    if (t->data[cur].type == COMMA) {
      cur++;
      continue;
    }

    if (cur >= fromPos ||
        (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
      break;

    int pathStart = cur;
    cur++;
    while (cur < fromPos && t->data[cur].type == DOT) {
      cur++;
      if (cur >= fromPos ||
          (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
        break;
      cur++;
    }

    /* Wildcard: path.* [as alias] */
    if (cur < fromPos && t->data[cur].type == STAR) {
      char pathBuf[256] = {0};
      for (int i = pathStart; i < cur; i++) {
        if (t->data[i].type == DOT)
          continue;
        if (pathBuf[0])
          strcat(pathBuf, ".");
        strcat(pathBuf, t->data[i].value);
      }
      entries[*entryCount].pathNode =
          createString(r->node, pathBuf, NODE_LITERAL_ID);
      entries[*entryCount].aliasNode = -1;
      entries[*entryCount].isWildcard = true;
      cur++;

      /* Optional `as alias` after wildcard */
      if (cur < fromPos &&
          (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
           t->data[cur].type == LITERAL_ID) &&
          !strcmp(t->data[cur].value, "as")) {
        cur++;
        if (cur < fromPos &&
            (t->data[cur].type == IDENTIFIER ||
             t->data[cur].type == LITERAL_ID)) {
          entries[*entryCount].aliasNode =
              createString(r->node, t->data[cur].value, NODE_LITERAL_ID);
          cur++;
        }
      }
      (*entryCount)++;
      continue;
    }

    /* Regular entry: path [as alias] */
    {
      char pathBuf[256] = {0};
      for (int i = pathStart; i < cur; i++) {
        if (t->data[i].type == DOT)
          continue;
        if (pathBuf[0])
          strcat(pathBuf, ".");
        strcat(pathBuf, t->data[i].value);
      }

      entries[*entryCount].pathNode =
          createString(r->node, pathBuf, NODE_LITERAL_ID);
      entries[*entryCount].isWildcard = false;
      entries[*entryCount].aliasNode = -1;

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
          entries[*entryCount].aliasNode = createString(
              r->node, t->data[nextCur].value, NODE_LITERAL_ID);
          nextCur++;
        }
      }

      (*entryCount)++;
      cur = nextCur;
    }
  }
  return cur;
}

/* Handle flat import: `import a.create, b.login as auth, d.* from modules` */
int grammarParseFlatImport(Request *r, Token *t, int a, int b, int *pos) {
  int fromPos = -1;
  /* `a` already points past the `import` keyword to the first entry.
   * No extra +1 needed — grammarModuleDetectFlatImport expects the
   * first entry identifier (e.g. `b` in `b.*`). */
  int baseIdx = grammarModuleDetectFlatImport(t, a, b, &fromPos);
  if (baseIdx < 0 || fromPos < 0)
    return GRAMMAR_NO_MATCH;

  struct AstModuleImportEntry entries[64];
  int entryCount = 0;
  parseFlatImportEntries(r, t, baseIdx, fromPos, entries, &entryCount);

  if (entryCount == 0) {
    *pos = b;
    return GRAMMAR_NO_MATCH;
  }

  /* Parse `from PATH` */
  int fromPathEnd = -1;
  int fromStart = grammarModuleDetectFrom(t, fromPos, b, &fromPathEnd);
  if (fromStart < 0) {
    *pos = b;
    return GRAMMAR_NO_MATCH;
  }

  char *fromPath = grammarModuleBuildPath(t, fromStart, fromPathEnd);
  int basePath = fromPath ? createString(r->node, fromPath, NODE_LITERAL_ID)
                          : -1;
  free(fromPath);

  /* Optional `as alias` after from path */
  int aliasIdx = -1;
  int afterPath = fromPathEnd + 1;
  if (afterPath < b &&
      (t->data[afterPath].type == KEYWORD ||
       t->data[afterPath].type == IDENTIFIER ||
       t->data[afterPath].type == LITERAL_ID) &&
      !strcmp(t->data[afterPath].value, "as")) {
    afterPath++;
    if (afterPath < b &&
        (t->data[afterPath].type == IDENTIFIER ||
         t->data[afterPath].type == LITERAL_ID)) {
      aliasIdx =
          createString(r->node, t->data[afterPath].value, NODE_LITERAL_ID);
      afterPath++;
    }
  }

  *pos = afterPath;
  return createModuleImport(r->node, basePath, entries, entryCount, aliasIdx);
}

/* Handle old-style import: `import X, Y from rupa.MODULE` or `from path` */
int grammarParseOldImport(Request *r, Token *t, int a, int b, int *pos) {
  int names[64];
  int nameCount = 0;
  int cur = a + 1;

  while (cur < b) {
    /* Check for `from rupa` (stdlib root, no dot) */
    int rupaRoot = grammarModuleDetectFromRupaRoot(t, cur, b);
    if (rupaRoot >= 0) {
      if (nameCount == 0) break;
      int nameIds[64];
      for (int i = 0; i < nameCount; i++)
        nameIds[i] = createString(r->node, t->data[names[i]].value,
                                  NODE_LITERAL_ID);
      int v = createArray(r->node, nameIds, nameCount);
      int n = createString(r->node, "rupa", NODE_LITERAL_ID);
      *pos = b;
      return createModule(r->node, NODE_IMPORT, v, n);
    }

    /* Check for `from rupa.M` (stdlib with dot) */
    int modIdx = grammarModuleDetectFromRupa(t, cur, b);
    if (modIdx >= 0) {
      if (nameCount == 0)
        break;
      if (nameCount == 1) {
        int v = createString(r->node, t->data[names[0]].value,
                             NODE_LITERAL_ID);
        int n = createString(r->node, t->data[modIdx].value, NODE_LITERAL_ID);
        *pos = b;
        return createModule(r->node, NODE_IMPORT, v, n);
      }
      int nameIds[64];
      for (int i = 0; i < nameCount; i++)
        nameIds[i] = createString(r->node, t->data[names[i]].value,
                                  NODE_LITERAL_ID);
      int v = createArray(r->node, nameIds, nameCount);
      int n = createString(r->node, t->data[modIdx].value, NODE_LITERAL_ID);
      *pos = b;
      return createModule(r->node, NODE_IMPORT, v, n);
    }

    /* Check for general `from X.Y.Z` (local file import) */
    {
      int pathEnd = -1;
      int pathStart = grammarModuleDetectFrom(t, cur, b, &pathEnd);
      if (pathStart >= 0 && pathEnd >= 0) {
        if (nameCount == 0)
          break;
        char *fullPath = grammarModuleBuildPath(t, pathStart, pathEnd);
        if (nameCount == 1) {
          int v = createString(r->node, t->data[names[0]].value,
                               NODE_LITERAL_ID);
          int n = fullPath ? createString(r->node, fullPath, NODE_LITERAL_ID)
                            : -1;
          free(fullPath);
          *pos = b;
          return createModule(r->node, NODE_IMPORT, v, n);
        }
        int nameIds[64];
        for (int i = 0; i < nameCount; i++)
          nameIds[i] = createString(r->node, t->data[names[i]].value,
                                    NODE_LITERAL_ID);
        int v = createArray(r->node, nameIds, nameCount);
        int n = fullPath ? createString(r->node, fullPath, NODE_LITERAL_ID)
                          : -1;
        free(fullPath);
        *pos = b;
        return createModule(r->node, NODE_IMPORT, v, n);
      }
    }

    if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID)
      break;

    names[nameCount++] = cur;
    cur++;

    if (cur < b && t->data[cur].type == COMMA)
      cur++;
    continue;
  }

  return GRAMMAR_NO_MATCH;
}
