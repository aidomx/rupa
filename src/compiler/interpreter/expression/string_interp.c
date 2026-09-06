#include <rupa.h>

/**
 * Interpret a NODE_STRING_INTERP: evaluates each part (string literal or
 * expression) and concatenates them into a single string result.
 *
 * Parts array alternates between NODE_STRING (literal text) and expression
 * nodes (evaluated at runtime).
 */
InterpreterResult interpretStringInterp(Node *node, AstNode *ast,
                                        RuntimeEnv *env, Error *error) {
  if (!ast || ast->type != NODE_STRING_INTERP)
    return resultNormal(valueNull());

  int partsLen = ast->stringInterp.length;
  if (partsLen == 0)
    return resultNormal(valueString("\"\""));

  /* Estimate initial capacity */
  size_t cap = 256;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf) return resultNormal(valueNull());
  buf[0] = '\0';

  /* Helper: ensure buffer has room for `need` more chars */
  #define ENSURE(need) do { \
    if (len + (need) + 1 > cap) { \
      cap = (len + (need) + 1) * 2; \
      buf = realloc(buf, cap); \
      if (!buf) return resultNormal(valueNull()); \
    } \
  } while(0)

  for (int i = 0; i < partsLen; i++) {
    int partId = ast->stringInterp.parts[i];
    if (partId < 0 || partId >= node->length)
      continue;

    AstNode *part = &node->ast[partId];

    if (part->type == NODE_STRING) {
      /* Literal string part: strip quotes and append */
      const char *raw = part->string.value;
      if (raw) {
        /* Strip surrounding quotes if present */
        size_t rlen = strlen(raw);
        const char *start = raw;
        const char *end = raw + rlen;
        if (rlen >= 2 && ((raw[0] == '"' && raw[rlen-1] == '"') ||
                          (raw[0] == '\'' && raw[rlen-1] == '\''))) {
          start = raw + 1;
          end = raw + rlen - 1;
        }
        int slen = (int)(end - start);
        ENSURE(slen);
        memcpy(buf + len, start, slen);
        len += slen;
        buf[len] = '\0';
      }
    } else {
      /* Expression part: evaluate it */
      InterpreterResult r = interpretNode(node, partId, env, error);
      if (r.flow != FLOW_NORMAL) {
        free(buf);
        return r;
      }

      /* Convert value to string representation and append */
      switch (r.value.type) {
      case VALUE_NUMBER: {
        char tmp[64];
        int n = snprintf(tmp, sizeof(tmp), "%d", r.value.as.number);
        ENSURE(n);
        memcpy(buf + len, tmp, n);
        len += n;
        break;
      }
      case VALUE_DECIMAL: {
        char tmp[64];
        int n = snprintf(tmp, sizeof(tmp), "%g", r.value.as.decimal);
        ENSURE(n);
        memcpy(buf + len, tmp, n);
        len += n;
        break;
      }
      case VALUE_BOOLEAN: {
        const char *s = r.value.as.boolean ? "true" : "false";
        int n = (int)strlen(s);
        ENSURE(n);
        memcpy(buf + len, s, n);
        len += n;
        break;
      }
      case VALUE_STRING: {
        if (r.value.as.string) {
          int n = (int)strlen(r.value.as.string);
          ENSURE(n);
          memcpy(buf + len, r.value.as.string, n);
          len += n;
        }
        break;
      }
      case VALUE_NULL: {
        const char *s = "null";
        int n = (int)strlen(s);
        ENSURE(n);
        memcpy(buf + len, s, n);
        len += n;
        break;
      }
      case VALUE_ARRAY: {
        /* Use valuePrint-like formatting */
        char tmp[256] = {0};
        tmp[0] = '[';
        int pos = 1;
        for (int j = 0; j < r.value.as.array.length && pos < 255; j++) {
          if (j > 0 && pos < 255) tmp[pos++] = ',';
          if (j > 0 && pos < 255) tmp[pos++] = ' ';
          RuntimeValue item = r.value.as.array.items[j];
          if (item.type == VALUE_NUMBER)
            pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%d", item.as.number);
          else if (item.type == VALUE_STRING && item.as.string)
            pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%s", item.as.string);
          else if (item.type == VALUE_BOOLEAN)
            pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%s", item.as.boolean ? "true" : "false");
          if (pos >= 255) break;
        }
        if (pos < 255) tmp[pos++] = ']';
        ENSURE(pos);
        memcpy(buf + len, tmp, pos);
        len += pos;
        break;
      }
      case VALUE_OBJECT: {
        const char *s = "[object]";
        int n = (int)strlen(s);
        ENSURE(n);
        memcpy(buf + len, s, n);
        len += n;
        break;
      }
      default: {
        const char *s = "[unknown]";
        int n = (int)strlen(s);
        ENSURE(n);
        memcpy(buf + len, s, n);
        len += n;
        break;
      }
      }
    }
  }

  buf[len] = '\0';

  #undef ENSURE

  /* Return as a quoted string that valueString will strip quotes from */
  char *quoted = malloc(len + 3);
  if (!quoted) { free(buf); return resultNormal(valueNull()); }
  quoted[0] = '"';
  memcpy(quoted + 1, buf, len);
  quoted[len + 1] = '"';
  quoted[len + 2] = '\0';
  free(buf);

  return resultNormal(valueString(quoted));
}
