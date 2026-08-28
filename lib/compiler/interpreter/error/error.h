#pragma once

#if defined(RUPA_PACKAGE_H)

Error *createError(int capacity);
void addError(Error *error, ErrorInfo errorInfo);
void addRuntimeError(Error *error, ErrorType type, const char *expected,
                     const char *actual);
void printErrors(const Error *error);
ErrorInfo setErrorInfo(const char *code, char *message, int line, int row,
                       ErrorType type);

#endif
