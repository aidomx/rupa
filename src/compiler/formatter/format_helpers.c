#include <rupa.h>

void fmtIndent(Formatter *f) {
  for (int i = 0; i < f->indent; i++)
    fprintf(f->out, "  ");
  f->needsIndent = false;
}

void fmtStr(Formatter *f, const char *s) {
  if (f->needsIndent) fmtIndent(f);
  fprintf(f->out, "%s", s);
}

void fmtChar(Formatter *f, char c) {
  if (f->needsIndent) fmtIndent(f);
  fprintf(f->out, "%c", c);
}

void fmtNewline(Formatter *f) {
  fprintf(f->out, "\n");
  f->needsIndent = true;
  f->lastWasNewline = true;
}

void fmtSep(Formatter *f) {
  fprintf(f->out, " ");
}
