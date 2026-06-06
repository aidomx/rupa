#pragma once

#if defined(RUPA_PACKAGE_H)

Error *createError(int capacity);
void addError(Error *error, ErrorInfo errorInfo);
ErrorInfo setErrorInfo(const char *code, char *message, int line, int row,
                       ErrorType type);

#endif
