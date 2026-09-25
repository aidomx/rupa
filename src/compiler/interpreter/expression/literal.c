#include <rupa.h>

/**
 * Process escape sequences in a string literal.
 * Converts "\n" to newline, "\t" to tab, "\\" to backslash, etc.
 * Returns a newly allocated string. Caller must free.
 */
static char *unescapeString(const char *src) {
  if (!src) return strdup("");

  /* Worst case: every char is escaped, so 2x the length */
  size_t len = strlen(src);
  char *buf = malloc(len * 2 + 1);
  if (!buf) return strdup("");

  char *dst = buf;
  const char *p = src;

  while (*p) {
    if (*p == '\\' && *(p + 1)) {
      p++;
      switch (*p) {
      case 'n':  *dst++ = '\n'; break;
      case 't':  *dst++ = '\t'; break;
      case 'r':  *dst++ = '\r'; break;
      case '\\': *dst++ = '\\'; break;
      case '"':  *dst++ = '"';  break;
      case '\'': *dst++ = '\''; break;
      case '0':  *dst++ = '\0'; break;
      default:
        /* Unknown escape: keep as-is (backslash + char) */
        *dst++ = '\\';
        *dst++ = *p;
        break;
      }
      p++;
    } else {
      *dst++ = *p++;
    }
  }
  *dst = '\0';
  return buf;
}

/* ===== String interning literal =====
 * Setiap evaluasi NODE_STRING dulu: unescapeString (malloc) + valueString
 * (gcmall + copy) + free — padahal literal yang sama dievaluasi berulang
 * di loop (tests/stress/max_loop.rp: 1M kali "...\n"). Cache berbasis
 * KONTEN raw literal: key = ast->string.value (didup — kehidupan AST
 * berakhir saat gcclean antar file, alamat bisa terpakai ulang), value =
 * buffer ter-unescape malloc biasa (BUKAN registry GC — gcclean() antar
 * file runner/formatter tidak boleh melepas blok yang masih di-cache).
 * RuntimeValue menunjuk langsung buffer cache: zero-alloc, zero-copy per
 * evaluasi. Aman dibagi antar nilai: buffer VALUE_STRING tidak pernah
 * dimutasi in-place (semua operasi string menyalin via valueString). */

struct StringInternEntry {
  char *key; /* salinan raw literal (milik cache) */
  size_t key_len;
  char *val; /* hasil unescape + strip quote (milik cache) */
  struct StringInternEntry *next;
};

#define STRING_INTERN_BUCKETS 1024 /* power of two */

static struct StringInternEntry *g_intern[STRING_INTERN_BUCKETS];

static uint64_t strHash(const char *s, size_t len) {
  uint64_t x = 1469598103934665603ULL; /* FNV-1a offset basis */
  for (size_t i = 0; i < len; i++) {
    x ^= (unsigned char)s[i];
    x *= 1099511628211ULL;
  }
  return x;
}

/* Ambil buffer ter-unescape untuk raw literal — unescape SEKALI, pakai
 * ulang selamanya. Semantik identik jalur lama: unescapeString lalu strip
 * quote pembuka/penutup (semantik valueString). */
static char *internLiteral(const char *raw) {
  if (!raw) return strdup("");

  size_t len = strlen(raw);
  uint64_t h = strHash(raw, len) & (STRING_INTERN_BUCKETS - 1);
  for (struct StringInternEntry *e = g_intern[h]; e; e = e->next) {
    if (e->key_len == len && memcmp(e->key, raw, len) == 0) return e->val;
  }

  /* Belum ter-cache: hitung sekali. */
  char *un = unescapeString(raw);
  if (!un) return strdup("");

  size_t ulen = strlen(un);
  const char *s = un;
  const char *end = un + ulen;
  if (ulen >= 2 && ((un[0] == '"' && un[ulen - 1] == '"') ||
                    (un[0] == '\'' && un[ulen - 1] == '\''))) {
    s = un + 1;
    end = un + ulen - 1;
  }
  size_t flen = (size_t)(end - s);
  char *final = malloc(flen + 1);
  if (!final) {
    free(un);
    return strdup("");
  }
  memcpy(final, s, flen);
  final[flen] = '\0';
  free(un);

  struct StringInternEntry *e = malloc(sizeof(*e));
  if (!e) return final; /* tanpa cache: tetap benar, hanya lambat */
  e->key = malloc(len + 1);
  if (!e->key) {
    free(e);
    return final;
  }
  memcpy(e->key, raw, len + 1);
  e->key_len = len;
  e->val = final;
  e->next = g_intern[h];
  g_intern[h] = e;
  return final;
}

InterpreterResult interpretLiteral(Node *node, AstNode *ast) {
  (void)node;
  switch (ast->type) {
  case NODE_NUMBER: return resultNormal(valueNumber(ast->number.value));
  case NODE_DECIMAL: return resultNormal(valueDecimal(ast->decimal.value));
  case NODE_BOOLEAN: return resultNormal(valueBoolean(ast->boolean.value));
  case NODE_STRING: {
    /* Interned: buffer cache dipakai ulang antar evaluasi & antar nilai
     * runtime — tidak dialokasi ulang per iterasi loop. */
    return resultNormal((RuntimeValue){.type = VALUE_STRING,
                                       .as.string = internLiteral(ast->string.value)});
  }
  case NODE_NULLABLE: return resultNormal(valueNull());
  default: return resultNormal(valueNull());
  }
}
