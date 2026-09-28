#include <rupa.h>

#include "format_object.h"
#include "format_scan.h"

/* ============================================================
 * format_object.c — object literal canonicalization & collapse
 *
 * Bentuk inline object dikendalikan objectSpaceTrim:
 *   false -> { name: "rupa", age: 20 }
 *   true  -> {name:"rupa",age:20}
 * Whitespace milik ekspresi ({ value: x + y}) tetap dipertahankan.
 * ============================================================ */

char *fmtObjectInlineSource(const char *src, size_t len, size_t open, size_t close,
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

/*
 * Normalize object literals and empty blocks in-place in a working buffer.
 *
 * Nested objects are handled first. This prevents an outer object from
 * accidentally destroying the newlines belonging to a nested object that
 * itself could not be collapsed.
 */
char *fmtCollapseObjects(const char *src, size_t len, const FormatterConfig *c, size_t *outLen) {
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
