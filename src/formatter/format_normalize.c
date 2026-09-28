#include <rupa.h>

#include "format_object.h"
#include "format_scan.h"

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
 * Scanning helpers live in format_scan.c, object canonicalization in
 * format_object.c; the public API is declared in lib/formatter/formatter.h.
 */

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
