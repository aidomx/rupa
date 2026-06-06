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
