#include <rupa.h>

/*
 * Source normalization for the formatter.
 *
 * This file only handles rules that cannot safely be delegated to the AST
 * serializer when source-based formatting is active:
 *
 *   - indentation
 *   - maximum consecutive empty lines
 *   - empty block collapsing
 *   - object literal collapse
 *   - object literal inline spacing
 *
 * The public API is declared in lib/formatter/formatter.h.
 */

/* ========================================================================== */
/* Lexical helpers                                                            */
/* ========================================================================== */

static bool fmtFindMatchingBrace(const char *src, size_t len, size_t open, size_t *close) {
  int depth = 1;
  bool str = false;
  bool esc = false;
  bool lineComment = false;
  bool blockComment = false;
  char quote = 0;

  for (size_t i = open + 1; i < len; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    if (lineComment) {
      if (ch == '\n') lineComment = false;
      continue;
    }

    if (blockComment) {
      if (ch == '*' && nx == '/') {
        blockComment = false;
        i++;
      }
      continue;
    }

    if (str) {
      if (esc)
        esc = false;
      else if (ch == '\\')
        esc = true;
      else if (ch == quote)
        str = false;
      continue;
    }

    if (ch == '"' || ch == '\'') {
      str = true;
      quote = ch;
      continue;
    }

    if (ch == '#') {
      lineComment = true;
      continue;
    }

    if (ch == '/' && nx == '/') {
      lineComment = true;
      i++;
      continue;
    }

    if (ch == '/' && nx == '*') {
      blockComment = true;
      i++;
      continue;
    }

    if (ch == '{') {
      depth++;
      continue;
    }

    if (ch == '}') {
      depth--;
      if (depth == 0) {
        *close = i;
        return true;
      }
    }
  }

  return false;
}

static bool fmtBraceIsObject(const char *src, size_t len, size_t open, size_t close) {
  int depth = 1;
  bool str = false;
  bool esc = false;
  bool lineComment = false;
  bool blockComment = false;
  bool hasTopColon = false;
  bool hasComment = false;
  char quote = 0;

  for (size_t i = open + 1; i < close; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    if (lineComment) {
      if (ch == '\n') lineComment = false;
      continue;
    }

    if (blockComment) {
      if (ch == '*' && nx == '/') {
        blockComment = false;
        i++;
      }
      continue;
    }

    if (str) {
      if (esc)
        esc = false;
      else if (ch == '\\')
        esc = true;
      else if (ch == quote)
        str = false;
      continue;
    }

    if (ch == '"' || ch == '\'') {
      str = true;
      quote = ch;
      continue;
    }

    if (ch == '#') {
      hasComment = true;
      lineComment = true;
      continue;
    }

    if (ch == '/' && nx == '/') {
      hasComment = true;
      lineComment = true;
      i++;
      continue;
    }

    if (ch == '/' && nx == '*') {
      hasComment = true;
      blockComment = true;
      i++;
      continue;
    }

    if (ch == '{') {
      depth++;
      continue;
    }

    if (ch == '}') {
      depth--;
      continue;
    }

    if (ch == ':' && depth == 1) hasTopColon = true;
  }

  if (!hasTopColon || hasComment) return false;

  /*
   * A brace pair is considered an object literal only when it appears in an
   * expression-like context. Plain `{ ... }` blocks must remain untouched.
   */
  size_t p = open;
  while (p > 0 && isspace((unsigned char)src[p - 1]))
    p--;

  if (p == 0) return true;

  char prev = src[p - 1];

  if (prev == '=' || prev == '(' || prev == '[' || prev == ',' || prev == ':') return true;

  if (p >= 6 && !strncmp(src + p - 6, "return", 6)) return true;

  return false;
}

static bool fmtObjectHasNewline(const char *src, size_t open, size_t close) {
  for (size_t i = open + 1; i < close; i++) {
    if (src[i] == '\n') return true;
  }
  return false;
}

static bool fmtObjectIsEmpty(const char *src, size_t open, size_t close) {
  for (size_t i = open + 1; i < close; i++) {
    if (!isspace((unsigned char)src[i])) return false;
  }

  return true;
}

/*
 * Detect whether an object contains comments.
 *
 * Objects containing comments are deliberately not collapsed. Their physical
 * layout carries source information which the source normalizer should not
 * destroy.
 */
static bool fmtObjectHasComment(const char *src, size_t len, size_t open, size_t close) {
  bool str = false;
  bool esc = false;
  bool lineComment = false;
  bool blockComment = false;
  char quote = 0;

  for (size_t i = open + 1; i < close; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    if (lineComment) {
      if (ch == '\n') lineComment = false;
      continue;
    }

    if (blockComment) {
      if (ch == '*' && nx == '/') {
        blockComment = false;
        i++;
      }
      continue;
    }

    if (str) {
      if (esc)
        esc = false;
      else if (ch == '\\')
        esc = true;
      else if (ch == quote)
        str = false;
      continue;
    }

    if (ch == '"' || ch == '\'') {
      str = true;
      quote = ch;
      continue;
    }

    if (ch == '#') return true;

    if (ch == '/' && nx == '/') return true;

    if (ch == '/' && nx == '*') return true;
  }

  return false;
}

/* ========================================================================== */
/* Object inline canonicalization                                             */
/* ========================================================================== */

/*
 * Canonicalize one object literal into its inline representation.
 *
 * The important distinction here is that objectSpaceTrim controls only the
 * spaces belonging to the object syntax:
 *
 *   false -> { name: "rupa", age: 20 }
 *   true  -> {name:"rupa",age:20}
 *
 * Whitespace belonging to an expression is preserved:
 *
 *   { value: x + y }
 *   {value:x + y}
 *
 * This is intentionally different from simply deleting every whitespace
 * character in the object.
 */
static char *fmtObjectInlineSource(const char *src, size_t len, size_t open, size_t close,
                                   const FormatterConfig *c, size_t *outLen) {
  if (!src || !c || open >= len || close >= len || open >= close || !outLen) return NULL;

  size_t sourceLen = close - open + 1;
  size_t cap = sourceLen + 64;
  char *out = malloc(cap);
  if (!out) return NULL;

  size_t w = 0;
  bool trim = c->objectSpaceTrim;
  bool str = false;
  bool esc = false;
  char quote = 0;

  int depth = 1;
  bool pendingSpace = false;
  bool wroteValue = false;

  /*
   * Append one character while keeping the buffer growth local to this
   * helper. The source is never larger than the original object plus a small
   * amount of canonical spacing, so geometric growth is sufficient.
   */
#define FMT_OBJECT_GROW(needed)                                                                    \
  do {                                                                                             \
    size_t _need = (needed);                                                                       \
    if (w + _need + 1 > cap) {                                                                     \
      size_t _newCap = cap * 2;                                                                    \
      while (_newCap < w + _need + 1)                                                              \
        _newCap *= 2;                                                                              \
      char *_tmp = realloc(out, _newCap);                                                          \
      if (!_tmp) {                                                                                 \
        free(out);                                                                                 \
        return NULL;                                                                               \
      }                                                                                            \
      out = _tmp;                                                                                  \
      cap = _newCap;                                                                               \
    }                                                                                              \
  } while (0)

#define FMT_OBJECT_PUT(ch)                                                                         \
  do {                                                                                             \
    FMT_OBJECT_GROW(1);                                                                            \
    out[w++] = (ch);                                                                               \
  } while (0)

  FMT_OBJECT_PUT('{');

  /*
   * Empty object.
   */
  bool empty = true;
  for (size_t i = open + 1; i < close; i++) {
    if (!isspace((unsigned char)src[i])) {
      empty = false;
      break;
    }
  }

  if (empty) {
    FMT_OBJECT_PUT('}');
    out[w] = '\0';
    *outLen = w;
    return out;
  }

  if (!trim) FMT_OBJECT_PUT(' ');

  for (size_t i = open + 1; i < close; i++) {
    char ch = src[i];
    char nx = (i + 1 < close) ? src[i + 1] : 0;

    if (str) {
      FMT_OBJECT_PUT(ch);

      if (esc)
        esc = false;
      else if (ch == '\\')
        esc = true;
      else if (ch == quote)
        str = false;

      continue;
    }

    if (ch == '"' || ch == '\'') {
      if (pendingSpace && wroteValue && !trim && w > 0 && out[w - 1] != ' ') {
        FMT_OBJECT_PUT(' ');
      }

      pendingSpace = false;
      FMT_OBJECT_PUT(ch);
      str = true;
      quote = ch;
      wroteValue = true;
      continue;
    }

    /*
     * Whitespace outside strings is delayed. We decide whether it belongs
     * to the object syntax when the next meaningful character arrives.
     */
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
      pendingSpace = true;
      continue;
    }

    /*
     * Nested braces belong to nested expressions/objects. Their own
     * formatter normalization is handled separately.
     */
    if (ch == '{') {
      if (pendingSpace && wroteValue && !trim) FMT_OBJECT_PUT(' ');

      pendingSpace = false;
      FMT_OBJECT_PUT(ch);
      depth++;
      wroteValue = true;
      continue;
    }

    if (ch == '}') {
      /*
       * This should normally only be reached for malformed/nested source
       * because close is the outer matching brace.
       */
      if (depth > 1) {
        if (pendingSpace && wroteValue && !trim) FMT_OBJECT_PUT(' ');

        pendingSpace = false;
        FMT_OBJECT_PUT(ch);
        depth--;
        wroteValue = true;
        continue;
      }
    }

    /*
     * Only punctuation at the current object level is controlled here.
     */
    if (ch == ':' && depth == 1) {
      while (w > 0 && out[w - 1] == ' ')
        w--;

      FMT_OBJECT_PUT(':');
      pendingSpace = false;

      if (!trim) FMT_OBJECT_PUT(' ');

      wroteValue = true;
      continue;
    }

    if (ch == ',' && depth == 1) {
      while (w > 0 && out[w - 1] == ' ')
        w--;

      FMT_OBJECT_PUT(',');
      pendingSpace = false;

      if (!trim) FMT_OBJECT_PUT(' ');

      wroteValue = true;
      continue;
    }

    /*
     * Ordinary expression whitespace is retained as a single space.
     *
     * For spaceTrim=true this is still retained because it belongs to the
     * expression rather than to the object delimiters themselves.
     */
    if (pendingSpace) {
      char prev = w > 0 ? out[w - 1] : 0;

      if (prev != '{' && prev != ':' && prev != ',' &&
          (prev == '\n' || prev == '\r' || prev == '\t')) {
        FMT_OBJECT_PUT(' ');
      }

      pendingSpace = false;
    }

    FMT_OBJECT_PUT(ch);
    wroteValue = true;

    (void)nx;
  }

  /*
   * Remove whitespace that accumulated immediately before the closing brace.
   */
  while (w > 0 && out[w - 1] == ' ')
    w--;

  if (!trim && w > 1) FMT_OBJECT_PUT(' ');

  FMT_OBJECT_PUT('}');

  out[w] = '\0';
  *outLen = w;

#undef FMT_OBJECT_GROW
#undef FMT_OBJECT_PUT

  return out;
}

/* ========================================================================== */
/* Source compliance                                                          */
/* ========================================================================== */

bool fmtSourceConfigCompliant(const char *src, size_t len, const FormatterConfig *c) {
  if (!src || !c) return true;

  int depth = 0;
  int lineDepth = 0;
  int emptyRun = 0;
  bool inString = false;
  char quote = 0;
  bool escape = false;
  bool lineComment = false;
  bool blockComment = false;
  int lineStart = 0;

  /*
   * Check indentation and empty-line rules.
   */
  for (size_t i = 0; i <= len; i++) {
    char ch = i < len ? src[i] : '\n';
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    if (lineComment) {
      if (ch == '\n')
        lineComment = false;
      else
        continue;
    }

    if (blockComment) {
      if (ch == '*' && nx == '/') {
        blockComment = false;
        i++;
      }
      continue;
    }

    if (inString) {
      if (escape)
        escape = false;
      else if (ch == '\\')
        escape = true;
      else if (ch == quote)
        inString = false;
      continue;
    }

    if (ch == '"' || ch == '\'') {
      inString = true;
      quote = ch;
      continue;
    }

    if (ch == '#') {
      lineComment = true;
      continue;
    }

    if (ch == '/' && nx == '/') {
      lineComment = true;
      i++;
      continue;
    }

    if (ch == '/' && nx == '*') {
      blockComment = true;
      i++;
      continue;
    }

    if (ch == '\n' || i == len) {
      int p = lineStart;

      while (p < (int)i && (src[p] == ' ' || src[p] == '\t' || src[p] == '\r')) {
        p++;
      }

      bool blank = p >= (int)i;

      if (blank) {
        emptyRun++;
        if (emptyRun > c->maxEmpty) return false;
      } else {
        emptyRun = 0;

        int expectedDepth = lineDepth;

        if (src[p] == '}') {
          expectedDepth = lineDepth > 0 ? lineDepth - 1 : 0;
        }

        int spaces = p - lineStart;

        /*
         * Continuation lines inside [] are not governed by block indentation.
         */
        if (spaces != expectedDepth * c->indentWidth) {
          bool inBracket = false;
          int bracketDepth = 0;

          for (int k = lineStart - 1; k >= 0; k--) {
            char z = src[k];

            if (z == '\n') break;

            if (z == ']') {
              bracketDepth++;
            } else if (z == '[') {
              if (bracketDepth > 0) {
                bracketDepth--;
              } else {
                inBracket = true;
                break;
              }
            }
          }

          if (!inBracket) return false;
        }
      }

      lineStart = (int)i + 1;
      lineDepth = depth;
      continue;
    }

    if (ch == '{') {
      depth++;
    } else if (ch == '}') {
      if (depth > 0) depth--;
    }
  }

  if (len == 0 || src[len - 1] != '\n') return false;

  /*
   * Inspect brace pairs for object-literal and empty-block rules.
   */
  for (size_t i = 0; i < len; i++) {
    if (src[i] != '{') continue;

    size_t close = 0;

    if (!fmtFindMatchingBrace(src, len, i, &close)) continue;

    bool object = fmtBraceIsObject(src, len, i, close);
    bool multiline = fmtObjectHasNewline(src, i, close);

    /*
     * Empty block/object handling.
     */
    if (fmtObjectIsEmpty(src, i, close)) {
      if (multiline && !c->keepEmptyBlock) return false;

      i = close;
      continue;
    }

    if (object) {
      /*
       * A multiline object with collapse enabled must collapse when its
       * canonical inline representation fits the configured limit.
       */
      if (multiline && c->objectCollapse && !fmtObjectHasComment(src, len, i, close)) {
        size_t inlineLen = 0;
        char *inlineObject = fmtObjectInlineSource(src, len, i, close, c, &inlineLen);

        if (!inlineObject) return false;

        bool fits = c->objectLimit <= 0 || (int)inlineLen <= c->objectLimit;

        free(inlineObject);

        if (fits) return false;
      }

      /*
       * Inline object must already have the canonical object spacing.
       *
       * Only compare the object itself. Spacing outside the object belongs
       * to its containing expression and is handled elsewhere.
       */
      if (!multiline) {
        size_t canonicalLen = 0;
        char *canonical = fmtObjectInlineSource(src, len, i, close, c, &canonicalLen);

        if (!canonical) return false;

        size_t sourceObjectLen = close - i + 1;

        bool same =
            canonicalLen == sourceObjectLen && memcmp(canonical, src + i, sourceObjectLen) == 0;

        free(canonical);

        if (!same) return false;
      }
    }

    i = close;
  }

  return true;
}

/* ========================================================================== */
/* Object normalization                                                       */
/* ========================================================================== */

/*
 * Normalize object literals and empty blocks in-place in a working buffer.
 *
 * Nested objects are handled first. This prevents an outer object from
 * accidentally destroying the newlines belonging to a nested object that
 * itself could not be collapsed.
 */
static char *fmtCollapseObjects(const char *src, size_t len, const FormatterConfig *c,
                                size_t *outLen) {
  if (!src || !c || !outLen) return NULL;

  char *cur = malloc(len + 1);

  if (!cur) return NULL;

  memcpy(cur, src, len);
  cur[len] = '\0';

  size_t curLen = len;

  for (;;) {
    bool changed = false;

    for (size_t i = 0; i < curLen; i++) {
      if (cur[i] != '{') continue;

      size_t close = 0;

      if (!fmtFindMatchingBrace(cur, curLen, i, &close)) continue;

      /*
       * Do not process an outer pair before its nested braces.
       */
      bool hasNestedBrace = false;

      {
        bool str = false;
        bool esc = false;
        bool lineComment = false;
        bool blockComment = false;
        char quote = 0;

        for (size_t k = i + 1; k < close; k++) {
          char ch = cur[k];
          char nx = (k + 1 < close) ? cur[k + 1] : 0;

          if (lineComment) {
            if (ch == '\n') lineComment = false;
            continue;
          }

          if (blockComment) {
            if (ch == '*' && nx == '/') {
              blockComment = false;
              k++;
            }
            continue;
          }

          if (str) {
            if (esc)
              esc = false;
            else if (ch == '\\')
              esc = true;
            else if (ch == quote)
              str = false;
            continue;
          }

          if (ch == '"' || ch == '\'') {
            str = true;
            quote = ch;
            continue;
          }

          if (ch == '#') {
            lineComment = true;
            continue;
          }

          if (ch == '/' && nx == '/') {
            lineComment = true;
            k++;
            continue;
          }

          if (ch == '/' && nx == '*') {
            blockComment = true;
            k++;
            continue;
          }

          if (ch == '{') {
            hasNestedBrace = true;
            break;
          }
        }
      }

      if (hasNestedBrace) {
        i = close;
        continue;
      }

      bool empty = fmtObjectIsEmpty(cur, i, close);

      if (close == i + 1) {
        i = close;
        continue;
      }

      /*
       * Empty block/object.
       */
      if (empty && !c->keepEmptyBlock) {
        /*
         * `{ ... }` -> `{}`
         */
        size_t oldEnd = close + 1;
        size_t newEnd = i + 2;

        cur[i + 1] = '}';

        memmove(cur + newEnd, cur + oldEnd, curLen - oldEnd + 1);

        curLen -= oldEnd - newEnd;
        cur[curLen] = '\0';

        changed = true;
        break;
      }

      /*
       * Non-object blocks are left alone.
       */
      if (!fmtBraceIsObject(cur, curLen, i, close)) {
        i = close;
        continue;
      }

      /*
       * Object containing comments must retain its source layout.
       */
      if (fmtObjectHasComment(cur, curLen, i, close)) {
        i = close;
        continue;
      }

      bool multiline = fmtObjectHasNewline(cur, i, close);

      /*
       * Multiline object.
       *
       * collapse=false means preserve its multiline representation.
       */
      if (multiline) {
        if (!c->objectCollapse) {
          i = close;
          continue;
        }

        size_t inlineLen = 0;
        char *inlineObject = fmtObjectInlineSource(cur, curLen, i, close, c, &inlineLen);

        if (!inlineObject) {
          free(cur);
          return NULL;
        }

        bool fits = c->objectLimit <= 0 || (int)inlineLen <= c->objectLimit;

        if (!fits) {
          free(inlineObject);
          i = close;
          continue;
        }

        size_t oldEnd = close + 1;

        memmove(cur + i, inlineObject, inlineLen);

        size_t newEnd = i + inlineLen;

        memmove(cur + newEnd, cur + oldEnd, curLen - oldEnd + 1);

        curLen -= oldEnd - newEnd;

        cur[curLen] = '\0';

        free(inlineObject);

        changed = true;
        break;
      }

      /*
       * Already-inline object.
       *
       * Normalize its configured spacing even when no collapse is necessary.
       */
      {
        size_t inlineLen = 0;
        char *inlineObject = fmtObjectInlineSource(cur, curLen, i, close, c, &inlineLen);

        if (!inlineObject) {
          free(cur);
          return NULL;
        }

        size_t oldEnd = close + 1;
        size_t oldLen = oldEnd - i;

        if (inlineLen != oldLen || memcmp(cur + i, inlineObject, inlineLen) != 0) {
          memmove(cur + i, inlineObject, inlineLen);

          size_t newEnd = i + inlineLen;

          memmove(cur + newEnd, cur + oldEnd, curLen - oldEnd + 1);

          curLen -= oldEnd - newEnd;
          cur[curLen] = '\0';

          changed = true;
        }

        free(inlineObject);

        if (changed) break;
      }

      i = close;
    }

    if (!changed) break;
  }

  *outLen = curLen;
  return cur;
}

/* ========================================================================== */
/* Full source normalization                                                  */
/* ========================================================================== */

char *fmtNormalizeSource(const char *src, size_t len, const FormatterConfig *c, size_t *outLen) {
  if (!src || !c || !outLen) return NULL;

  size_t normalizedLen = 0;

  char *work = fmtCollapseObjects(src, len, c, &normalizedLen);

  if (!work) return NULL;

  size_t cap = normalizedLen + normalizedLen / 4 + 256;

  if (cap < 256) cap = 256;

  char *out = malloc(cap);

  if (!out) {
    free(work);
    return NULL;
  }

  size_t w = 0;

  int depth = 0;
  int emptyRun = 0;
  int bracket = 0;

  bool str = false;
  bool esc = false;
  bool lineComment = false;
  bool blockComment = false;
  bool previousColonLine = false;

  char quote = 0;

  size_t line = 0;

  while (line < normalizedLen) {
    size_t end = line;

    while (end < normalizedLen && work[end] != '\n')
      end++;

    size_t p = line;

    while (p < end && (work[p] == ' ' || work[p] == '\t' || work[p] == '\r')) {
      p++;
    }

    bool blank = p == end;
    bool lineInsideBlockComment = blockComment;

    if (blank) {
      if (emptyRun < c->maxEmpty) {
        if (w + 1 >= cap) {
          cap *= 2;
          char *tmp = realloc(out, cap);
          if (!tmp) {
            free(out);
            free(work);
            return NULL;
          }
          out = tmp;
        }

        out[w++] = '\n';
        emptyRun++;
      }

      line = end + (end < normalizedLen ? 1 : 0);
      continue;
    }

    emptyRun = 0;

    int expected = depth;

    if (work[p] == '}') expected = depth > 0 ? depth - 1 : 0;

    /*
     * Colon-form statements (`if x:` etc.) do not define block indentation
     * in this source normalizer.
     */
    if (previousColonLine && depth == 0 && bracket == 0) expected = -1;

    if (lineInsideBlockComment) expected = -1;

    /*
     * Array continuation indentation is intentionally outside the configured
     * block indentation policy.
     */
    if (bracket > 0) expected = -1;

    if (expected >= 0) {
      size_t count = (size_t)expected * (size_t)c->indentWidth;

      if (w + count + 1 >= cap) {
        while (w + count + 1 >= cap)
          cap *= 2;

        char *tmp = realloc(out, cap);

        if (!tmp) {
          free(out);
          free(work);
          return NULL;
        }

        out = tmp;
      }

      for (size_t k = 0; k < count; k++)
        out[w++] = ' ';
    } else {
      /*
       * Preserve indentation for continuation lines which are not governed
       * by block indentation.
       */
      size_t lead = p - line;

      if (w + lead + 1 >= cap) {
        while (w + lead + 1 >= cap)
          cap *= 2;

        char *tmp = realloc(out, cap);

        if (!tmp) {
          free(out);
          free(work);
          return NULL;
        }

        out = tmp;
      }

      memcpy(out + w, work + line, lead);
      w += lead;
    }

    size_t contentLen = end - p;

    if (w + contentLen + 2 >= cap) {
      while (w + contentLen + 2 >= cap)
        cap *= 2;

      char *tmp = realloc(out, cap);

      if (!tmp) {
        free(out);
        free(work);
        return NULL;
      }

      out = tmp;
    }

    memcpy(out + w, work + p, contentLen);
    w += contentLen;

    if (end < normalizedLen) out[w++] = '\n';

    /*
     * Update lexical state from the original normalized line.
     */
    for (size_t i = line; i < end; i++) {
      char ch = work[i];
      char nx = (i + 1 < end) ? work[i + 1] : 0;

      if (lineComment) continue;

      if (blockComment) {
        if (ch == '*' && nx == '/') {
          blockComment = false;
          i++;
        }
        continue;
      }

      if (str) {
        if (esc)
          esc = false;
        else if (ch == '\\')
          esc = true;
        else if (ch == quote)
          str = false;

        continue;
      }

      if (ch == '"' || ch == '\'') {
        str = true;
        quote = ch;
        continue;
      }

      if (ch == '#') {
        lineComment = true;
        continue;
      }

      if (ch == '/' && nx == '/') {
        lineComment = true;
        i++;
        continue;
      }

      if (ch == '/' && nx == '*') {
        blockComment = true;
        i++;
        continue;
      }

      if (ch == '{') {
        depth++;
      } else if (ch == '}') {
        if (depth > 0) depth--;
      } else if (ch == '[') {
        bracket++;
      } else if (ch == ']') {
        if (bracket > 0) bracket--;
      }
    }

    lineComment = false;

    /*
     * Detect colon-form continuation for the next physical line.
     */
    size_t q = end;

    while (q > line && isspace((unsigned char)work[q - 1])) {
      q--;
    }

    previousColonLine = q > line && work[q - 1] == ':';

    line = end + (end < normalizedLen ? 1 : 0);
  }

  free(work);

  /*
   * Formatter output always ends with exactly one final newline.
   */
  if (w == 0 || out[w - 1] != '\n') {
    if (w + 1 >= cap) {
      cap++;
      char *tmp = realloc(out, cap);

      if (!tmp) {
        free(out);
        return NULL;
      }

      out = tmp;
    }

    out[w++] = '\n';
  }

  out[w] = '\0';
  *outLen = w;

  return out;
}
