#include <rupa.h>

#include "format_scan.h"

/* ============================================================
 * format_scan.c — lexical scanner formatter
 *
 * State machine kecil untuk membaca source tanpa tersandung
 * string/escape/komentar. Dipakai lintas unit normalisasi;
 * kontrak ada di format_scan.h.
 * ============================================================ */

/* Status lexical satu titik pemindaian — dipakai helper di bawah
 * agar state machine string/komentar tidak diduplikasi. */
typedef struct {
  bool str;
  bool esc;
  bool lineComment;
  bool blockComment;
  char quote;
} FmtLexState;

/* Maju satu karakter pada state (tanpa memproses `ch` sebagai
 * struktur brace). Return true bila karakter adalah struktur
 * (di luar string & komentar) dan boleh dipakai pemanggil. */
static bool fmtLexStep(FmtLexState *s, char ch, char nx) {
  if (s->lineComment) {
    if (ch == '\n') s->lineComment = false;
    return false;
  }

  if (s->blockComment) {
    if (ch == '*' && nx == '/') {
      s->blockComment = false;
      return false; /* pemanggil tetap skip karakter ini */
    }
    return false;
  }

  if (s->str) {
    if (s->esc)
      s->esc = false;
    else if (ch == '\\')
      s->esc = true;
    else if (ch == s->quote)
      s->str = false;
    return false;
  }

  if (ch == '"' || ch == '\'') {
    s->str = true;
    s->quote = ch;
    return false;
  }

  if (ch == '#') {
    s->lineComment = true;
    return false;
  }

  if (ch == '/' && nx == '/') {
    s->lineComment = true;
    return false;
  }

  if (ch == '/' && nx == '*') {
    s->blockComment = true;
    return false;
  }

  return true;
}

bool fmtFindMatchingBrace(const char *src, size_t len, size_t open, size_t *close) {
  int depth = 1;
  FmtLexState s = {0};

  for (size_t i = open + 1; i < len; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    bool structural = fmtLexStep(&s, ch, nx);

    /* Karakter kedua penanda komentar (// atau ...) ikut dikonsumsi
     * state machine lewat nx; di sini cukup lewati bila non-struktural. */
    if (!structural) continue;

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

bool fmtBraceIsObject(const char *src, size_t len, size_t open, size_t close) {
  int depth = 1;
  FmtLexState s = {0};
  bool hasTopColon = false;

  for (size_t i = open + 1; i < close; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    if (!fmtLexStep(&s, ch, nx)) continue;

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

  if (!hasTopColon || s.lineComment) return false;

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

bool fmtObjectHasNewline(const char *src, size_t open, size_t close) {
  for (size_t i = open + 1; i < close; i++) {
    if (src[i] == '\n') return true;
  }
  return false;
}

bool fmtObjectIsEmpty(const char *src, size_t open, size_t close) {
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
bool fmtObjectHasComment(const char *src, size_t len, size_t open, size_t close) {
  FmtLexState s = {0};

  for (size_t i = open + 1; i < close; i++) {
    char ch = src[i];
    char nx = (i + 1 < len) ? src[i + 1] : 0;

    /* Deteksi komentar harus berjalan SEBELUM fmtLexStep menelan
       state-nya: '#' dan '//' memicu lineComment, '/*' blockComment. */
    if (!s.str && !s.lineComment && !s.blockComment) {
      if (ch == '#') return true;
      if (ch == '/' && nx == '/') return true;
      if (ch == '/' && nx == '*') return true;
    }

    fmtLexStep(&s, ch, nx);
  }

  return false;
}
