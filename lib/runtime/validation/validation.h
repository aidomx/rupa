#pragma once

#if defined(RUPA_PACKAGE_H)

// utility syntax
extern void annotationType(const char *buffer, int *position);
extern int braces(const char *buffer, int start, int *level);
extern int consumeToEnd(const char *buffer, int pos);
extern int parenthesis(const char *buffer, int start, int *level);
extern void skipWhitespace(const char *buffer, int *position);
extern void subscripts(const char *buffer, int *position);

// expression syntax
extern bool isLeftExpression(Context *ctx, ValidationInput *vi,
                             const char *buffer, int *position);
extern bool isRightExpression(Context *ctx, ValidationInput *vi,
                              const char *buffer, int *position);

extern void resetValidate(ValidationInput *vi);

// keyword syntax
extern bool isValidPrint(const char *buffer, int *position);
extern bool isValidBlock(const char *buffer, int *position);
extern bool isValidModule(const char *buffer, int *position);

// program syntax
extern void setNextProgram(const char *buffer, Context *ctx, int *position,
                           ValidationInput *vi);

// main
ValidationInput *createValidationInput(size_t size);
ValidationInput *isCompleteInput(Input *input);
extern void *processValidationInput(Input *input);

#endif
