#include <rupa.h>

ErrorInfo *createErrorInfo(int capacity) {
  ErrorInfo *info = gcmall(capacity * sizeof(ErrorInfo));
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
    free(error->info);
    return NULL;
  }

  error->capacity = capacity;
  error->size = 0;
  return error;
}

void addError(Error *error, ErrorInfo info) {
  if (!error || !error->info || error->size >= error->capacity)
    return;

  /* ErrorInfo hanya di-shallow-copy di sini; kalau caller mengirim
   * `.message` dari buffer yang dipakai ulang (mis. `static char
   * message[256]` seperti di interpretIdentifier()/addRuntimeError()),
   * SEMUA entry yang pernah menyimpan pointer ke buffer itu akan ikut
   * berubah begitu buffer ditulis ulang oleh panggilan berikutnya -
   * simtomnya: error pertama ("first is not defined") ikut berubah jadi
   * isi error kedua ("second is not defined") saat keduanya baru benar-
   * benar dibaca/dicetak belakangan. Duplikasi di sini supaya setiap
   * entry punya salinannya sendiri yang tidak berubah lagi setelahnya. */
  if (info.message)
    info.message = gcstrdup(info.message);

  error->info[error->size++] = info;
}

void addRuntimeError(Error *error, ErrorType type, const char *expected,
                     const char *actual) {
  if (!error || error->size >= error->capacity)
    return;

  static char message[256];
  snprintf(message, sizeof(message), "Type mismatch: expected '%s', got '%s'",
           expected ? expected : "unknown", actual ? actual : "unknown");
  ErrorInfo info = {.code = "TypeError", .message = message, .line = 0,
                    .row = 0, .type = type};
  addError(error, info);
}

void printErrors(const Error *error) {
  if (!error) return;
  for (int i = 0; i < error->size; i++) {
    const ErrorInfo *info = &error->info[i];
    fprintf(stderr, "%s: %s\n", info->code ? info->code : "Error",
            info->message ? info->message : "unknown error");
  }
}
