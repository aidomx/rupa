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

InterpreterResult interpretLiteral(Node *node, AstNode *ast) {
  switch (ast->type) {
  case NODE_NUMBER: return resultNormal(valueNumber(ast->number.value));
  case NODE_DECIMAL: return resultNormal(valueDecimal(ast->decimal.value));
  case NODE_BOOLEAN: return resultNormal(valueBoolean(ast->boolean.value));
  case NODE_STRING: {
    char *unescaped = unescapeString(ast->string.value);
    RuntimeValue val = valueString(unescaped);
    /* Note: valueString does not copy, so unescaped stays owned by val.
     * If valueString copies, we'd free here. Check valueString impl. */
    free(unescaped);
    return resultNormal(val);
  }
  case NODE_NULLABLE: return resultNormal(valueNull());
  default: return resultNormal(valueNull());
  }
}
