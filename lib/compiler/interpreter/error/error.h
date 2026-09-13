#pragma once

#if defined(RUPA_PACKAGE_H)

Error *createError(int capacity);
void addError(Error *error, ErrorInfo errorInfo);
void addRuntimeError(Error *error, ErrorType type, const char *expected,
                     const char *actual);
void printErrors(const Error *error);
void addSourceError(Error *error, const char *code, const char *message,
                    int line, int row, ErrorType type);
void addSourceErrorAt(Error *error, const char *code, const char *message,
                      const char *source, int pos, ErrorType type);
void setRuntimeErrorLocation(int line, int row);
ErrorInfo setErrorInfo(const char *code, char *message, int line, int row,
                       ErrorType type);

#endif
