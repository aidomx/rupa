#ifndef RUPA_FORMATTER_OBJECT_H
#define RUPA_FORMATTER_OBJECT_H

#include <rupa.h>

/*
 * format_object.h — kontrak internal unit object literal formatter.
 */

/* Kanonikalkan satu object literal ke bentuk inline sesuai config
 * (objectSpaceTrim / spacing). Hasil malloc; pemanggil yang free. */
char *fmtObjectInlineSource(const char *src, size_t len, size_t open, size_t close,
                            const FormatterConfig *c, size_t *outLen);

/* Collapse object literal & empty block berulang sampai stabil.
 * Nested diproses lebih dulu agar newline object dalam yang tak bisa
 * collapse tidak ikut hancur. Hasil malloc; pemanggil yang free. */
char *fmtCollapseObjects(const char *src, size_t len, const FormatterConfig *c, size_t *outLen);

#endif /* RUPA_FORMATTER_OBJECT_H */
