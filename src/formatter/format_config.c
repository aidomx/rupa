#include <rupa.h>

/*
 * Formatter configuration: memuat & mem-parsing .rupa-format (bagian dari
 * struktur modular formatter - lihat komentar di format_dispatch.c untuk
 * peta seluruh file).
 */

/* ==================== Formatter configuration ==================== */

static FormatterConfig fmtConfigDefaults(void) {
  FormatterConfig c = {
      .indentWidth = 2,
      .maxEmpty = 1,
      .objectLimit = 100,
      .objectCollapse = true,
      .objectSpaceTrim = false,
      .keepEmptyBlock = false,
      .classKeepEmptyBlock = false,
      .classMemberKeepEmptyBlock = false,
      .classMemberKeepEmptyMember = false,
  };
  return c;
}

static char *fmtTrim(char *s) {
  while (*s && isspace((unsigned char)*s))
    s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1]))
    --end;
  *end = '\0';
  return s;
}

static bool fmtParseBool(const char *s, bool *out) {
  if (!s || !out) return false;
  if (strcmp(s, "true") == 0 || strcmp(s, "yes") == 0) {
    *out = true;
    return true;
  }
  if (strcmp(s, "false") == 0 || strcmp(s, "no") == 0) {
    *out = false;
    return true;
  }
  return false;
}

static bool fmtParsePositiveInt(const char *s, int *out) {
  if (!s || !*s || !out) return false;
  char *end = NULL;
  long n = strtol(s, &end, 10);
  if (end == s || *end != '\0' || n < 0 || n > INT_MAX) return false;
  *out = (int)n;
  return true;
}

/*
 * .rupa-format is intentionally a tiny YAML-like configuration file. We only
 * parse the subset Rupa owns: sections, one nested `members` section, and
 * scalar key/value pairs. Unknown options remain harmless for forward
 * compatibility.
 */
static void fmtConfigApply(FormatterConfig *c, const char *section, const char *subsection,
                           const char *key, const char *value) {
  if (!c || !key || !value) return;
  int n;
  bool b;

  if (strcmp(section, "Indentation") == 0) {
    if (strcmp(key, "width") == 0 && fmtParsePositiveInt(value, &n)) c->indentWidth = n;
    return;
  }

  if (strcmp(section, "Lines") == 0) {
    if (strcmp(key, "maxEmpty") == 0 && fmtParsePositiveInt(value, &n)) c->maxEmpty = n;
    return;
  }

  if (strcmp(section, "ObjectLiteral") == 0) {
    if (strcmp(key, "limit") == 0 && fmtParsePositiveInt(value, &n))
      c->objectLimit = n;
    else if (strcmp(key, "collapse") == 0 && fmtParseBool(value, &b))
      c->objectCollapse = b;
    else if (strcmp(key, "spaceTrim") == 0 && fmtParseBool(value, &b))
      c->objectSpaceTrim = b;

    return;
  }

  if (strcmp(section, "Blocks") == 0) {
    if (strcmp(key, "keepEmpty") == 0 && fmtParseBool(value, &b)) c->keepEmptyBlock = b;
    return;
  }

  if (strcmp(section, "ClassBlock") == 0) {
    if (!subsection) {
      if (strcmp(key, "keepEmpty") == 0 && fmtParseBool(value, &b)) c->classKeepEmptyBlock = b;
      return;
    }
    if (strcmp(subsection, "members") == 0) {
      if (strcmp(key, "keepEmptyBlock") == 0 && fmtParseBool(value, &b))
        c->classMemberKeepEmptyBlock = b;
      else if (strcmp(key, "keepEmptyMember") == 0 && fmtParseBool(value, &b))
        c->classMemberKeepEmptyMember = b;
    }
  }
}

FormatterConfig fmtLoadConfig(void) {
  FormatterConfig c = fmtConfigDefaults();
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) return c;

  FILE *fp = NULL;
  char configPath[PATH_MAX + 32];
  char *dir = cwd;
  for (;;) {
    snprintf(configPath, sizeof(configPath), "%s/.rupa-format", dir);
    fp = fopen(configPath, "rb");
    if (fp) break;
    if (strcmp(dir, "/") == 0) break;
    char *slash = strrchr(dir, '/');
    if (!slash) break;
    if (slash == dir) {
      dir[1] = '\0';
    } else {
      *slash = '\0';
    }
  }
  if (!fp) return c;

  char line[1024];
  char section[64] = {0};
  char subsection[64] = {0};
  while (fgets(line, sizeof(line), fp)) {
    char *raw = line;
    char *comment = strchr(raw, '#');
    if (comment) *comment = '\0';
    char *text = fmtTrim(raw);
    if (!*text) continue;

    int indent = 0;
    for (char *q = raw; *q && (*q == ' ' || *q == '\t'); q++)
      indent += (*q == '\t') ? 2 : 1;

    if (text[0] == '-') {
      text = fmtTrim(text + 1);
    }

    char *colon = strchr(text, ':');
    if (!colon) continue;
    *colon = '\0';
    char *key = fmtTrim(text);
    char *value = fmtTrim(colon + 1);

    if (!*value) {
      if (indent == 0) {
        snprintf(section, sizeof(section), "%s", key);
        subsection[0] = '\0';
      } else {
        snprintf(subsection, sizeof(subsection), "%s", key);
      }
      continue;
    }

    fmtConfigApply(&c, section, subsection[0] ? subsection : NULL, key, value);
  }
  fclose(fp);
  return c;
}
