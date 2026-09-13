#include <rupa.h>

/*
 * Import grammar — emits NODE_MOD (AstMod, ImportDecl) for all forms:
 *
 *   Flat:     import a.create, b.login as auth, d.* from modules as m
 *   Stdlib:   import X, Y from rupa.MODULE
 *             import X from rupa
 *   Old:      import X, Y from path.to.file
 *   Bare:     import X            (bind module X directly)
 */

/* Build an entry from a dotted path ("a", "a.create", "a.b.c").
 * Wildcard entries keep the full path as name (formatter adds `.*`). */
AstModEntry *modEntryFromPath(const char *path, bool wild) {
  if (wild) return modEntryWild(path);

  char parts[16][64];
  int count = 0;
  const char *p = path;
  while (*p && count < 16) {
    int i = 0;
    while (*p && *p != '.' && i < 63)
      parts[count][i++] = *p++;
    parts[count][i] = '\0';
    count++;
    if (*p == '.') p++;
  }
  if (count == 0) return NULL;
  if (count == 1) return modEntry(parts[0]);

  AstModEntry *chain = NULL, *tail = NULL;
  for (int i = 1; i < count; i++) {
    AstModEntry *c = modEntry(parts[i]);
    if (!c) continue;
    if (tail)
      tail->childrens = c;
    else
      chain = c;
    tail = c;
  }
  return modEntryMember(parts[0], chain);
}

/* Parse flat import entries between baseIdx and fromPos. */
static int parseFlatImportEntries(Request *r, Token *t, int baseIdx, int fromPos,
                                  AstModEntry **entries, int *entryCount) {
  (void)r;
  int cur = baseIdx;
  *entryCount = 0;

  while (cur < fromPos) {
    if (t->data[cur].type == COMMA) {
      cur++;
      continue;
    }

    if (cur >= fromPos || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
      break;

    int pathStart = cur;
    cur++;
    while (cur < fromPos && t->data[cur].type == DOT) {
      cur++;
      if (cur >= fromPos || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
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
    if (cur < fromPos && t->data[cur].type == STAR) {
      wild = true;
      cur++;
    }

    AstModEntry *e = modEntryFromPath(pathBuf, wild);
    if (!e) break;
    entries[(*entryCount)++] = e;

    /* Optional `as alias` */
    if (cur < fromPos &&
        (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "as")) {
      cur++;
      if (cur < fromPos && (t->data[cur].type == IDENTIFIER || t->data[cur].type == LITERAL_ID)) {
        e->key = gcstrdup(t->data[cur].value);
        cur++;
      }
      /* Trailing wildcard after alias: `a as form.*` → wildcard entry */
      if (cur + 1 < fromPos && t->data[cur].type == DOT && t->data[cur + 1].type == STAR) {
        e->type = MOD_WILD;
        cur += 2;
      }
    }
  }
  return cur;
}

/* Handle flat import: `import a.create, b.login as auth, d.* from modules as m` */
int grammarParseFlatImport(Request *r, Token *t, int a, int b, int *pos) {
  int fromPos = -1;
  /* `a` already points past the `import` keyword to the first entry.
   * No extra +1 needed — grammarModuleDetectFlatImport expects the
   * first entry identifier (e.g. `b` in `b.*`). */
  int baseIdx = grammarModuleDetectFlatImport(t, a, b, &fromPos);
  if (baseIdx < 0 || fromPos < 0) return GRAMMAR_NO_MATCH;

  AstModEntry *entries[64];
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

  /* Optional `as alias` after from path */
  char aliasBuf[128] = {0};
  int afterPath = fromPathEnd + 1;
  if (afterPath < b &&
      (t->data[afterPath].type == KEYWORD || t->data[afterPath].type == IDENTIFIER ||
       t->data[afterPath].type == LITERAL_ID) &&
      !strcmp(t->data[afterPath].value, "as")) {
    afterPath++;
    if (afterPath < b &&
        (t->data[afterPath].type == IDENTIFIER || t->data[afterPath].type == LITERAL_ID)) {
      snprintf(aliasBuf, sizeof(aliasBuf), "%s", t->data[afterPath].value);
      afterPath++;
    }
  }

  *pos = afterPath;
  int id = createModImport(r->node, entries, entryCount, fromPath, aliasBuf[0] ? aliasBuf : NULL);
  free(fromPath);
  return id;
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
      AstModEntry *entries[64];
      for (int i = 0; i < nameCount; i++)
        entries[i] = modEntry(t->data[names[i]].value);
      *pos = b;
      return createModImport(r->node, entries, nameCount, "rupa", NULL);
    }

    /* Check for `from rupa.M` (stdlib with dot) */
    int modIdx = grammarModuleDetectFromRupa(t, cur, b);
    if (modIdx >= 0) {
      if (nameCount == 0) break;
      AstModEntry *entries[64];
      for (int i = 0; i < nameCount; i++)
        entries[i] = modEntry(t->data[names[i]].value);
      char fullPath[512];
      snprintf(fullPath, sizeof(fullPath), "rupa.%s", t->data[modIdx].value);
      *pos = b;
      return createModImport(r->node, entries, nameCount, fullPath, NULL);
    }

    /* Check for general `from X.Y.Z` (local file import) */
    {
      int pathEnd = -1;
      int pathStart = grammarModuleDetectFrom(t, cur, b, &pathEnd);
      if (pathStart >= 0 && pathEnd >= 0) {
        if (nameCount == 0) break;
        AstModEntry *entries[64];
        for (int i = 0; i < nameCount; i++)
          entries[i] = modEntry(t->data[names[i]].value);
        char *fullPath = grammarModuleBuildPath(t, pathStart, pathEnd);
        *pos = b;
        int id = createModImport(r->node, entries, nameCount, fullPath, NULL);
        free(fullPath);
        return id;
      }
    }

    if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID) break;

    names[nameCount++] = cur;
    cur++;

    if (cur < b && t->data[cur].type == COMMA) cur++;
    continue;
  }

  return GRAMMAR_NO_MATCH;
}

/* Handle bare import: `import X` (no `from`) — bind module X directly. */
int grammarParseBareImport(Request *r, Token *t, int a, int b, int *pos) {
  int cur = a + 1;
  if (cur >= b) return GRAMMAR_NO_MATCH;
  if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID) return GRAMMAR_NO_MATCH;

  AstModEntry *entries[64];
  int entryCount = 0;
  entries[entryCount++] = modEntry(t->data[cur].value);
  cur++;

  *pos = cur;
  return createModImport(r->node, entries, entryCount, NULL, NULL);
}
