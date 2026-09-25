#include <rupa.h>

/*
 * Import grammar — emits NODE_MOD (AstMod, ImportDecl) for the canonical
 * forms (design/ie.txt):
 *
 *   import x                  — bare, file/folder sejajar (satu folder)
 *   import * as x from ./X    — whole module sebagai namespace
 *   import x, y from ./X      — selective member
 *   import x as y from ./X    — selective dengan alias per-entry
 *   import X from rupa(.Y)    — stdlib
 *   import z from ./X.Y.Z     — dual resolution: leaf dulu, lalu parent index
 *
 * Member chain (x.y) dan wildcard member (x.*) tetap dihapus. Bare
 * `import X` untuk siklus sejajar (satu folder, ie.txt import #1).
 */

/* Parse flat import entries between baseIdx and fromPos.
 * Menerima: leading `*` (optional `as alias`) dan identifier polos
 * dipisah koma — masing-masing boleh diikuti `as alias` (ie.txt: `as`
 * didukung penuh per-entry). Member chain dan wildcard member bukan
 * grammar. */
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

    /* Leading wildcard: `import * as x from ./X` — whole-module alias.
     * Entry bernama kosong + key alias; runtime memakai source langsung. */
    if (t->data[cur].type == STAR) {
      cur++;
      char *alias = NULL;
      if (cur < fromPos &&
          (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
           t->data[cur].type == LITERAL_ID) &&
          !strcmp(t->data[cur].value, "as")) {
        cur++;
        if (cur < fromPos &&
            (t->data[cur].type == IDENTIFIER || t->data[cur].type == LITERAL_ID)) {
          alias = gcstrdup(t->data[cur].value);
          cur++;
        }
      }
      AstModEntry *e = modEntry("");
      if (!e) break;
      e->key = alias; /* NULL jika tanpa alias */
      e->type = MOD_WILD;
      entries[(*entryCount)++] = e;
      continue;
    }

    if (cur >= fromPos || (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID))
      break;

    AstModEntry *e = modEntry(t->data[cur].value);
    cur++;
    /* `as alias` per-entry: `import x as y from ./X`. */
    if (cur < fromPos &&
        (t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "as")) {
      cur++;
      if (cur < fromPos &&
          (t->data[cur].type == IDENTIFIER || t->data[cur].type == LITERAL_ID)) {
        if (e) e->key = gcstrdup(t->data[cur].value);
        cur++;
      }
    }
    entries[(*entryCount)++] = e;
  }
  return cur;
}

/* Handle flat import: `import a, b as c, * as m from ./X` */
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

  /* Source alias (from X as m) dihapus dari grammar — bentuk warisan.
   * Path berhenti di token yang bukan bagian path (from path hanya
   * terdiri dari IDENTIFIER/LITERAL_ID/DOT/SLASH). */
  *pos = fromPathEnd + 1;
  int id = createModImport(r->node, entries, entryCount, fromPath, NULL);
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

    /* Check for general `from X.Y.Z` (local file import). Dotted path
     * tetap utuh sebagai nama source — runtime menafsirkannya sebagai
     * navigasi pohon package (redirect ke parent index). */
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
    /* `as` bukan nama: alias per-entry ditangani flat parser; old parser
     * tidak menelannya sebagai entry sampah. */
    if (!strcmp(t->data[cur].value, "as")) break;

    names[nameCount++] = cur;
    cur++;

    if (cur < b && t->data[cur].type == COMMA) cur++;
    continue;
  }

  return GRAMMAR_NO_MATCH;
}

/* Handle bare import: `import X` — siklus sejajar (satu folder,
 * design/import_export.txt §Bare). Muat ./X.rp atau ./X/index.rp;
 * runtime yang menegakkan batasan dan melaporkan error bila tidak
 * ada keduanya. Hanya satu nama polos — bentuk lain bukan grammar. */
int grammarParseBareImport(Request *r, Token *t, int a, int b, int *pos) {
  int cur = a + 1;
  if (cur >= b) return GRAMMAR_NO_MATCH;
  if (t->data[cur].type != IDENTIFIER && t->data[cur].type != LITERAL_ID)
    return GRAMMAR_NO_MATCH;
  /* Nama kedua (comma/dot/keyword apa pun) → bukan bare import. */
  if (cur + 1 < b && t->data[cur + 1].type != NEWLINE && t->data[cur + 1].type != ENDOF)
    return GRAMMAR_NO_MATCH;

  AstModEntry *entries[64];
  int entryCount = 0;
  entries[entryCount++] = modEntry(t->data[cur].value);

  *pos = cur + 1;
  return createModImport(r->node, entries, entryCount, NULL, NULL);
}
