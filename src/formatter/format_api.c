#include <rupa.h>

/*
 * Entry point publik formatter: formatFile/formatString/formatStdin.
 */

/* ==================== Public API ==================== */

int formatFile(const char *path) {
  /* Single-file mode keeps the raw formatter output (no batch status). */
  return fmtRunFile(path, stdout);
}

int formatString(const char *source) {
  if (!source || !*source) return 1;

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) return 1;

  Buffer *buffer = state->buffer;
  int len = strlen(source);
  if (buffer->capacity < len + 1) {
    buffer->capacity = len + 256;
    buffer->value = realloc(buffer->value, buffer->capacity);
  }
  memcpy(buffer->value, source, len);
  buffer->value[len] = '\0';
  buffer->length = len;

  FormatterConfig config = fmtLoadConfig();
  if (fmtSourceConfigCompliant(buffer->value, (size_t)buffer->length, &config)) {
    fwrite(buffer->value, 1, (size_t)buffer->length, stdout);
    gcclean();
    return 0;
  }

  Formatter fmt = {0};
  fmt.out = stdout;
  fmt.indent = 0;
  fmt.needsIndent = false;
  fmt.lastWasNewline = false;
  fmt.config = &config;

  int result = runFormat(state, &fmt);
  gcclean();
  return result;
}

int formatStdin(void) {
  char buf[65536];
  int total = 0;
  int n;
  while ((n = fread(buf + total, 1, sizeof(buf) - total - 1, stdin)) > 0)
    total += n;
  buf[total] = '\0';
  if (total == 0) return 0;
  return formatString(buf);
}
