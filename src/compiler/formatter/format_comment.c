#include <rupa.h>

/*
 * Format and output a comment at source[*pos].
 * Advances *pos past the comment.
 * Comment types: hashtag, double-slash, block
 */

/* Output a line, stripping \r and converting \t to spaces. */
static void fmtCleanLine(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    if (s[i] == '\r')
      continue;
    if (s[i] == '\t')
      fprintf(stdout, "    ");
    else
      fputc(s[i], stdout);
  }
}

void formatComment(const char *source, int *pos, int length) {
  char c = source[*pos];
  char next = (*pos + 1 < length) ? source[*pos + 1] : 0;

  /* Single-line comment: # */
  if (c == '#') {
    int start = *pos;
    while (*pos < length && source[*pos] != '\n')
      (*pos)++;
    /* Strip trailing \r */
    int end = *pos;
    while (end > start && source[end - 1] == '\r')
      end--;
    fprintf(stdout, "%.*s\n", end - start, source + start);
    return;
  }

  /* Single-line comment: // */
  if (c == '/' && next == '/') {
    int start = *pos;
    while (*pos < length && source[*pos] != '\n')
      (*pos)++;
    int end = *pos;
    while (end > start && source[end - 1] == '\r')
      end--;
    fprintf(stdout, "%.*s\n", end - start, source + start);
    return;
  }

  /*
   * Block comment: slash-star ... star-slash
   */
  if (c == '/' && next == '*') {
    int start = *pos;
    (*pos) += 2;
    while (*pos < length - 1) {
      if (source[*pos] == '*' && source[*pos + 1] == '/') {
        (*pos) += 2;
        break;
      }
      (*pos)++;
    }
    /* If unterminated, advance to end */
    if (*pos > length)
      *pos = length;

    int commentLen = *pos - start;

    /* Check if single-line block: no newlines between delimiters */
    bool hasNewline = false;
    for (int i = 0; i < commentLen; i++) {
      char ch = source[start + i];
      if (ch == '\n' || ch == '\r') {
        hasNewline = true;
        break;
      }
    }

    if (!hasNewline) {
      /* Single-line block comment: output as-is, strip \r */
      int outLen = commentLen;
      while (outLen > 0 && (source[start + outLen - 1] == '\n' ||
                            source[start + outLen - 1] == '\r'))
        outLen--;
      fmtCleanLine(source + start, outLen);
      fputc('\n', stdout);
      return;
    }

    /* Multi-line block comment: format with alignment.
     * First line as-is, subsequent lines prefixed with star-space,
     * closing star-slash at column 0. */
    int lineStart = 0;
    bool first = true;
    for (int i = 0; i <= commentLen; i++) {
      char ch = (i < commentLen) ? source[start + i] : '\n';
      if (ch == '\n' || ch == '\r') {
        /* Skip \r before \n */
        if (ch == '\r' && i + 1 < commentLen && source[start + i + 1] == '\n')
          i++;
        int lineLen = i - lineStart;
        const char *line = source + start + lineStart;

        if (first) {
          /* First line: output as-is (clean) */
          fmtCleanLine(line, lineLen);
          fputc('\n', stdout);
          first = false;
        } else {
          /* Trim leading whitespace (space, tab, \r) */
          int p = 0;
          while (p < lineLen && (line[p] == ' ' || line[p] == '\t' ||
                                line[p] == '\r'))
            p++;

          if (p < lineLen && line[p] == '*' && p + 1 < lineLen &&
              line[p + 1] == '/') {
            /* Closing star-slash -- output with leading space */
            fprintf(stdout, " */\n");
          } else if (p < lineLen && line[p] == '*') {
            /* Line with star -- ensure space after star */
            fprintf(stdout, " *");
            /* Output remaining content after star, cleaned */
            int contentStart = p + 1;
            int contentLen = lineLen - contentStart;
            /* Trim trailing \r */
            while (contentLen > 0 && line[contentStart + contentLen - 1] == '\r')
              contentLen--;
            fmtCleanLine(line + contentStart, contentLen);
            fputc('\n', stdout);
          } else {
            /* No star -- add star-space prefix */
            int contentLen = lineLen - p;
            /* Trim trailing \r */
            while (contentLen > 0 && line[p + contentLen - 1] == '\r')
              contentLen--;
            if (contentLen > 0) {
              fprintf(stdout, " * ");
              fmtCleanLine(line + p, contentLen);
            } else {
              fprintf(stdout, " *");
            }
            fputc('\n', stdout);
          }
        }
        lineStart = i + 1;
      }
    }
    return;
  }
}
