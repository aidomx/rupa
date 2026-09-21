#include <rupa.h>

static int g_error_line = 0;
static int g_error_row = 0;

ErrorInfo *createErrorInfo(int capacity) {
  ErrorInfo *info = gcmall(capacity * sizeof(ErrorInfo));
  info->file = NULL;
  info->code = NULL;
  info->message = NULL;
  info->line = 1;
  info->row = 0;
  info->type = ERROR_NONE;
  return info;
}

Error *createError(int capacity) {
  Error *error = gcmall(sizeof(Error));
  error->info = createErrorInfo(capacity);
  if (!error->info) {
    gcfree(error->info);
    return NULL;
  }

  error->capacity = capacity;
  error->size = 0;
  return error;
}

void addError(Error *error, ErrorInfo info) {
  if (!error || !error->info || error->size >= error->capacity) return;

  /* ErrorInfo hanya di-shallow-copy di sini; kalau caller mengirim
   * `.message` dari buffer yang dipakai ulang (mis. `static char
   * message[256]` seperti di interpretIdentifier()/addRuntimeError()),
   * SEMUA entry yang pernah menyimpan pointer ke buffer itu akan ikut
   * berubah begitu buffer ditulis ulang oleh panggilan berikutnya -
   * simtomnya: error pertama ("first is not defined") ikut berubah jadi
   * isi error kedua ("second is not defined") saat keduanya baru benar-
   * benar dibaca/dicetak belakangan. Duplikasi di sini supaya setiap
   * entry punya salinannya sendiri yang tidak berubah lagi setelahnya. */
  if (!info.file) info.file = getSourceFilePath();
  if (info.file) info.file = gcstrdup(info.file);
  if (info.line <= 0) info.line = g_error_line;
  if (info.row <= 0) info.row = g_error_row;
  if (info.message) info.message = gcstrdup(info.message);

  error->info[error->size++] = info;
}

void addRuntimeError(Error *error, ErrorType type, const char *expected, const char *actual) {
  if (!error || error->size >= error->capacity) return;

  static char message[MAX_MESSAGE_LENGTH];
  snprintf(message, MAX_MESSAGE_LENGTH, "Type mismatch: expected '%s', got '%s'",
           expected ? expected : "unknown", actual ? actual : "unknown");
  ErrorInfo info = {.file = getSourceFilePath(),
                    .code = "TypeError",
                    .message = message,
                    .line = 0,
                    .row = 0,
                    .type = type};
  addError(error, info);
}

void printErrors(const Error *error) {
  if (!error) return;
  /* Program output (stdout, line-buffered) harus sudah keluar sebelum
   * error (stderr, unbuffered) — tanpa flush, `print(5)` tanpa `\n`
   * masih di buffer dan error tampil menyalip di depan. */
  fflush(stdout);
  for (int i = 0; i < error->size; i++) {
    const ErrorInfo *info = &error->info[i];
    fprintf(stderr, "%s:%d:%d: %s: %s\n", info->file ? info->file : "<input>", info->line,
            info->row, info->code ? info->code : "Error",
            info->message ? info->message : "unknown error");
  }
}

ErrorInfo setErrorInfo(const char *code, char *message, int line, int row, ErrorType type) {
  return (ErrorInfo){.file = getSourceFilePath(),
                     .code = code,
                     .message = message,
                     .line = line,
                     .row = row,
                     .type = type};
}

void addSourceError(Error *error, const char *code, const char *message, int line, int row,
                    ErrorType type) {
  if (!error) return;
  addError(error, (ErrorInfo){.file = getSourceFilePath(),
                              .code = code,
                              .message = (char *)message,
                              .line = line,
                              .row = row,
                              .type = type});
}

void addSourceErrorAt(Error *error, const char *code, const char *message, const char *source,
                      int pos, ErrorType type) {
  if (!error) return;
  int line = 1, row = 1;
  if (source) {
    int len = (int)strlen(source);
    if (pos < 0) pos = 0;
    if (pos > len) pos = len;
    for (int i = 0; i < pos; i++) {
      if (source[i] == '\n') {
        line++;
        row = 1;
      } else {
        row++;
      }
    }
  }
  addSourceError(error, code, message, line, row, type);
}

void setRuntimeErrorLocation(int line, int row) {
  g_error_line = line;
  g_error_row = row;
}
