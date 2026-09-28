#ifndef RUPA_FORMATTER_SCAN_H
#define RUPA_FORMATTER_SCAN_H

#include <rupa.h>

/*
 * format_scan.h — kontrak internal unit lexical scanner formatter.
 *
 * State machine komentar/string/escape dipakai bersama oleh
 * format_normalize.c (compliance + normalize) dan format_object.c
 * (collapse object literal).
 */

/* Posisi `}` pasangan `{` di `open` (string & komentar dilewati).
 * false bila tidak seimbang. */
bool fmtFindMatchingBrace(const char *src, size_t len, size_t open, size_t *close);

/* Brace pair terlihat seperti object literal (ada `:` top-level dan
 * muncul di konteks ekspresi: = ( [ , : atau return). */
bool fmtBraceIsObject(const char *src, size_t len, size_t open, size_t close);

/* Object mengandung komentar — layout sumber tidak boleh diubah. */
bool fmtObjectHasComment(const char *src, size_t len, size_t open, size_t close);

/* Object kosong (hanya whitespace) atau berisi newline. */
bool fmtObjectIsEmpty(const char *src, size_t open, size_t close);
bool fmtObjectHasNewline(const char *src, size_t open, size_t close);

#endif /* RUPA_FORMATTER_SCAN_H */
