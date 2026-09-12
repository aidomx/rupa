#include <rupa.h>

/*
 * Shared helpers for module path detection, used by both import and export
 * grammar parsers. Extracted from grammar_module.c for modularity.
 */

/* Detect `from rupa` pattern (stdlib root, no dot after rupa). */
int grammarModuleDetectFromRupaRoot(Token *t, int from_pos, int limit) {
  if ((from_pos + 1) >= limit)
    return -1;
  if ((t->data[from_pos].type != IDENTIFIER &&
       t->data[from_pos].type != KEYWORD) ||
      strcmp(t->data[from_pos].value, "from"))
    return -1;
  if ((t->data[from_pos + 1].type != IDENTIFIER &&
       t->data[from_pos + 1].type != LITERAL_ID) ||
      strcmp(t->data[from_pos + 1].value, "rupa"))
    return -1;
  /* Must NOT be followed by a dot — `from rupa.X` is different */
  if ((from_pos + 2) < limit && t->data[from_pos + 2].type == DOT)
    return -1;
  /* Return position of 'rupa' */
  return from_pos + 1;
}

/* Detect `from rupa.MODULE` pattern (stdlib imports). */
int grammarModuleDetectFromRupa(Token *t, int from_pos, int limit) {
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

/* Detect general `from X.Y.Z` pattern (any module, not just rupa). */
int grammarModuleDetectFrom(Token *t, int from_pos, int limit, int *end) {
  if ((from_pos + 2) >= limit)
    return -1;
  if ((t->data[from_pos].type != IDENTIFIER &&
       t->data[from_pos].type != KEYWORD &&
       t->data[from_pos].type != LITERAL_ID) ||
      strcmp(t->data[from_pos].value, "from"))
    return -1;
  int mod_start = from_pos + 1;
  if (mod_start >= limit)
    return -1;
  int mod_end = mod_start;
  int cur = mod_start;
  while (cur < limit) {
    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER) &&
        !strcmp(t->data[cur].value, "as"))
      break;
    if (t->data[cur].type == COMMA)
      break;
    if (t->data[cur].type == IDENTIFIER || t->data[cur].type == LITERAL_ID ||
        t->data[cur].type == DOT || t->data[cur].type == SLASH) {
      mod_end = cur;
      cur++;
    } else {
      break;
    }
  }
  if (cur == mod_start)
    return -1;
  *end = mod_end;
  return mod_start;
}

/* Build concatenated module path string from tokens[start..end]. */
char *grammarModuleBuildPath(Token *t, int start, int end) {
  int total = 0;
  for (int i = start; i <= end; i++) {
    if (t->data[i].type == DOT) {
      if (i + 1 <= end &&
          (t->data[i + 1].type == SLASH || t->data[i + 1].type == DOT))
        total++;
    } else if (t->data[i].type == SLASH) {
      total++;
    } else {
      total += (int)strlen(t->data[i].value);
    }
  }
  int parts = 0;
  for (int i = start; i <= end; i++) {
    if (t->data[i].type == IDENTIFIER || t->data[i].type == LITERAL_ID)
      parts++;
  }
  if (parts > 1)
    total += (parts - 1);

  char *buf = malloc(total + 1);
  if (!buf)
    return NULL;
  buf[0] = '\0';
  int first = 1;
  for (int i = start; i <= end; i++) {
    if (t->data[i].type == DOT) {
      if (i + 1 <= end &&
          (t->data[i + 1].type == SLASH || t->data[i + 1].type == DOT)) {
        strcat(buf, ".");
        first = 0;
      }
    } else if (t->data[i].type == SLASH) {
      strcat(buf, "/");
      first = 0;
    } else {
      if (!first && (i == start || t->data[i - 1].type != SLASH))
        strcat(buf, ".");
      strcat(buf, t->data[i].value);
      first = 0;
    }
  }
  return buf;
}

/* Detect flat import syntax: `import a.create, b.login as auth, d.*` */
int grammarModuleDetectFlatImport(Token *t, int a, int b, int *fromPos) {
  if (a + 2 >= b)
    return -1;
  if (t->data[a].type != IDENTIFIER && t->data[a].type != LITERAL_ID)
    return -1;
  /* First entry must continue with `.` (a.create) or an `as` alias
   * (a as form.*) — old-style `import X from Y` falls through to the
   * legacy parser instead. */
  if (a + 1 >= b)
    return -1;
  TokenType next = t->data[a + 1].type;
  bool asAlias = (next == KEYWORD || next == IDENTIFIER || next == LITERAL_ID) &&
                 !strcmp(t->data[a + 1].value, "as");
  if (next != DOT && !asAlias)
    return -1;

  int cur = a;
  while (cur < b) {
    if ((t->data[cur].type == KEYWORD || t->data[cur].type == IDENTIFIER ||
         t->data[cur].type == LITERAL_ID) &&
        !strcmp(t->data[cur].value, "from")) {
      *fromPos = cur;
      return a;
    }
    /* Skip over any token that could appear in the entry list.
     * We check type FIRST to avoid strcmp on tokens that may not
     * have a valid string (e.g. DOT, STAR, COMMA). */
    TokenType q = t->data[cur].type;
    if (q == COMMA || q == IDENTIFIER || q == LITERAL_ID || q == DOT ||
        q == STAR || q == SLASH) {
      cur++;
      continue;
    }
    /* Keywords like 'as' also need to be skipped */
    if (q == KEYWORD) {
      cur++;
      continue;
    }
    break;
  }
  return -1;
}
