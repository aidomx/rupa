#include <rupa.h>

/*bool nextValidate(const char *buffer, Context *ctx, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return false;*/

/*if (!vi->isIdentifier)*/
/*return false;*/

/*if (vi->isAssignment) {*/
/*(*position)++;*/
/*skipWhitespace(buffer, position);*/

/*if (buffer[*position] == '\0')*/
/*return false;*/

/*printf("current ptr:%c\n", buffer[*position]);*/
/*}*/

/*if (vi->isAnnotionType) {*/
/*(*position)++;*/
/*annotationType(buffer, position);*/
/*if (isassign(buffer[*position])) {*/
/*printf("ptr on annotion type:%c\n", buffer[*position]);*/
/*}*/
/*}*/

/*if (vi->isBlockProgram) {*/
/*printf("block program\n");*/
/*return false;*/
/*}*/

/*if (vi->isFunction) {*/
/*parenthesis(buffer, (*position), &ctx->parenLevel);*/
/*skipWhitespace(buffer, position);*/
/*return false;*/
/*}*/

/*if (vi->isFunctionCall) {*/
/*printf("function call\n");*/
/*if (buffer[*position] == '\0') {*/
/*ctx->current = CONTEXT_FUNCTION_CALL;*/
/*return true;*/
/*}*/
/*return false;*/
/*}*/

/*if (vi->isFunctionDecl) {*/
/*printf("function decl\n");*/
/*(*position)++;*/
/*ctx->braceLevel++;*/

/*if (isrblock(buffer[*position])) {*/
/*ctx->braceLevel--;*/

/*if (ctx->braceLevel == 0) {*/
/*ctx->current = CONTEXT_FUNCTION_DECLARATION;*/
/*return true;*/
/*}*/
/*return false;*/
/*}*/
/*return false;*/
/*}*/

/*if (vi->isStringLiteral) {*/
/*printf("string literal:%c\n", buffer[*position]);*/
/*ctx->inQuotes = 1;*/
/*ctx->quoteChar = buffer[*position];*/
/*(*position)++;*/
/*while (buffer[*position] && buffer[*position] != ctx->quoteChar)*/
/*(*position)++;*/
/*if (buffer[*position] == ctx->quoteChar)*/
/*(*position)++;*/
/*ctx->inQuotes = 0;*/
/*ctx->quoteChar = '\0';*/
/*ctx->current = CONTEXT_STRING_LITERAL;*/

/*if (vi->isFunction)*/
/*return false;*/

/*return true;*/
/*}*/

/*if (vi->isStructDecl) {*/
/*printf("struct\n");*/
/*return false;*/
/*}*/

/*ctx->current = CONTEXT_VAR_DECLARATION;*/
/*return true;*/
/*}*/

/*void setIdentifier(const char *buffer, Context *ctx, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return;*/

/*int current_pos = *position;*/
/*if (!vi->isIdentifier) {*/
/*skipWhitespace(buffer, position);*/
/*generalIdentifier(buffer, position);*/

/*if (*position <= current_pos)*/
/*return;*/

/*ctx->current = CONTEXT_IDENTIFIER_REFERENCE;*/
/*vi->isIdentifier = true;*/
/*}*/
/*}*/

/*ValidationInput *setValidate(const char *buffer, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || *position == -1 || !vi)*/
/*return NULL;*/

/*skipWhitespace(buffer, position);*/
/*char next = buffer[*position];*/

/*printf("next:%c\n", next);*/

/*if (isassign(next)) {*/
/*vi->isAssignment = true;*/
/*return vi;*/
/*}*/

/*if (iscolon(next)) {*/
/*vi->isAnnotionType = true;*/
/*return vi;*/
/*}*/

/*if (islblock(next)) {*/
/*vi->isBlockProgram = true;*/
/*return vi;*/
/*}*/

/*if (islbracket(next)) {*/
/*vi->isArray = true;*/
/*return vi;*/
/*}*/

/*if (islparen(next)) {*/
/*vi->isFunction = true;*/
/*return vi;*/
/*}*/

/*if (isquote(next)) {*/
/*vi->isStringLiteral = true;*/
/*return vi;*/
/*}*/

/*return vi;*/
/*}*/

/*void isCompleteProgram(const char *buffer, Context *ctx, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return;*/

/*if (vi->isBlockProgram && !vi->isFunction) {*/
/*printf("is struct\n");*/
/*}*/

/*if (vi->isFunction) {*/
/*parenthesis(buffer, (*position), &ctx->parenLevel);*/
/*skipWhitespace(buffer, position);*/
/*printf("parenthesis:%d,char:%c\n", ctx->parenLevel, buffer[*position]);*/

/*if (!vi->isBlockProgram && ctx->parenLevel == 0)*/
/*printf("function call\n");*/

/*else*/
/*printf("function decl\n");*/
/*}*/

/*if (vi->isFunctionCall) {*/
/*vi->isComplete = !isrparen(buffer[*position - 1]) && ctx->parenLevel == 0;*/
/*vi->isFunction = vi->isComplete && false;*/
/*}*/

/*if (vi->isFunctionDecl) {*/
/*vi->isComplete = isrblock(buffer[*position]) && ctx->parenLevel == 0;*/
/*}*/
/*}*/

/*ValidationInput *isProgramValidate(const char *buffer, Context *ctx,*/
/*int *position, ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return NULL;*/

/*setIdentifier(buffer, ctx, position, vi);*/
/*skipWhitespace(buffer, position);*/
/*char next = buffer[*position];*/

/*printf("next:%c\n", next);*/

/*switch (next) {*/
/*case '[':*/
/*vi->isArray = true;*/
/*case '=':*/
/*vi->isAssignment = true;*/
/*case '{':*/
/*vi->isBlockProgram = true;*/
/*case '\0':*/
/*vi->isComplete = true;*/
/*case '(':*/
/*vi->isFunction = true;*/
/*case '"':*/
/*vi->isStringLiteral = true;*/
/*}*/

/*return vi;*/
/*}*/

/*void isExpressionValidation(Input *input) {*/
/*if (!input || !input->buffer || !input->context || input->cursor == -1 ||*/
/*!input->validation)*/
/*return;*/

/*int current_pos = input->cursor;*/

/*ValidationInput *vi = isProgramValidate(input->buffer, input->context,*/
/*&input->cursor, input->validation);*/

/*if (!vi)*/
/*return;*/

/*[>setIdentifier(buffer, ctx, &pos, input->validation);<]*/
/*[>ValidationInput *vi = setValidate(buffer, &pos, input->validation);<]*/

/*[>if (!vi)<]*/
/*[>return;<]*/

/*[>vi->isComplete = nextValidate(buffer, ctx, &pos, vi);<]*/

/*isCompleteProgram(input->buffer, input->context, &input->cursor,*/
/*input->validation);*/
/*input->cursor = current_pos;*/
/*}*/

/*// expression*/
/*bool isLeftExpression(Context *ctx, ValidationInput *vi, const char *buffer,*/
/*int *position) {*/
/*if (!ctx || !vi || !buffer || *position == -1)*/
/*return false;*/

/*int start = *position;*/
/*skipWhitespace(buffer, position);*/
/*generalIdentifier(buffer, position);*/

/*if (*position <= start)*/
/*return false; // tidak ada identifier valid*/

/*vi->isIdentifier = true;*/
/*skipWhitespace(buffer, position);*/
/*char next = buffer[*position];*/

/*[> ---- CASE: identifier = ... ---- <]*/
/*if (isassign(next)) {*/
/*(*position)++;*/
/*vi->isAssignment = true;*/
/*ctx->current = CONTEXT_ASSIGNMENT_EXPRESSION;*/
/*return true;*/
/*}*/

/*[> ---- CASE: identifier: string = ... ---- <]*/
/*if (iscolon(next)) {*/
/*(*position)++;*/
/*annotationType(buffer, position);*/
/*vi->isAnnotionType = true;*/
/*ctx->current = CONTEXT_TYPE_ANNOTATION;*/
/*return isLeftExpression(ctx, vi, buffer, position);*/
/*}*/

/*[> ---- CASE: identifier[...] ---- <]*/
/*if (islbracket(next)) {*/
/*(*position)++;*/
/*ctx->bracketLevel++;*/
/*subscripts(buffer, position);*/

/*if (isrbracket(buffer[*position])) {*/
/*ctx->bracketLevel--;*/
/*(*position)++;*/
/*if (isassign(buffer[*position])) {*/
/*vi->isArray = true;*/
/*vi->isAssignment = true;*/
/*ctx->current = CONTEXT_ARRAY_LITERAL;*/
/*}*/
/*}*/
/*return true;*/
/*}*/

/*[> ---- CASE: identifier() / identifier() {} ---- <]*/
/*if (islparen(next)) {*/
/*(*position)++;*/
/*parenthesis(buffer, ctx, position);*/

/*if (isrparen(buffer[*position])) {*/
/*ctx->parenLevel--;*/
/*(*position)++;*/
/*skipWhitespace(buffer, position);*/

/*if (islblock(buffer[*position])) {*/
/*(*position)++;*/
/*ctx->braceLevel++;*/
/*vi->isFunctionDecl = true;*/
/*ctx->current = CONTEXT_FUNCTION_DECLARATION;*/
/*} else {*/
/*vi->isFunctionCall = true;*/
/*ctx->current = CONTEXT_FUNCTION_CALL;*/
/*}*/
/*return true;*/
/*}*/
/*}*/

/*[> ---- CASE: identifier {} ---- <]*/
/*skipWhitespace(buffer, position);*/
/*if (islblock(buffer[*position])) {*/
/*(*position)++;*/
/*ctx->braceLevel++;*/
/*vi->isStructDecl = true;*/
/*ctx->current = CONTEXT_STRUCT_DECLARATION;*/
/*return true;*/
/*}*/

/*ctx->current = CONTEXT_VAR_DECLARATION;*/
/*return true;*/
/*}*/

/*bool isRightExpression(Context *ctx, ValidationInput *vi, const char
 * *buffer,*/
/*int *position) {*/
/*if (!ctx || !vi || !buffer || *position == -1)*/
/*return false;*/

/*skipWhitespace(buffer, position);*/
/*char c = buffer[*position];*/

/*if (isspace(c) || c == '\0') {*/
/*(*position)--;*/
/*if (isassign(buffer[*position])) {*/
/*return false;*/
/*}*/
/*return true;*/
/*}*/

/*if (isoperator(c)) {*/
/*ctx->current = CONTEXT_ARITHMETIC_EXPRESSION;*/
/*return true;*/
/*}*/

/*[> ---- String literal ---- <]*/
/*if (isquote(c)) {*/
/*ctx->inQuotes = 1;*/
/*ctx->quoteChar = c;*/
/*(*position)++;*/
/*while (buffer[*position] && buffer[*position] != ctx->quoteChar)*/
/*(*position)++;*/
/*if (buffer[*position] == ctx->quoteChar)*/
/*(*position)++;*/
/*ctx->inQuotes = 0;*/
/*ctx->quoteChar = '\0';*/
/*ctx->current = CONTEXT_STRING_LITERAL;*/
/*return true;*/
/*}*/

/*[> ---- Function or identifier ---- <]*/
/*if (isalpha(c) || isunderscore(c)) {*/
/*int start = *position;*/
/*generalIdentifier(buffer, position);*/

/*if (islparen(buffer[*position])) {*/
/*(*position)++;*/
/*parenthesis(buffer, ctx, position);*/

/*if (isrparen(buffer[*position])) {*/
/*ctx->parenLevel--;*/
/*(*position)++;*/
/*vi->isFunctionCall = true;*/
/*ctx->current = CONTEXT_FUNCTION_CALL;*/
/*}*/
/*return true;*/
/*}*/

/*if (*position > start) {*/
/*ctx->current = CONTEXT_IDENTIFIER_REFERENCE;*/
/*return true;*/
/*}*/
/*}*/

/*[> ---- Struct literal ---- <]*/
/*if (islblock(c)) {*/
/*ctx->braceLevel++;*/
/*(*position)++;*/
/*ctx->current = CONTEXT_STRUCT_ASSIGNMENT;*/
/*vi->isStructDecl = true;*/
/*return true;*/
/*}*/

/*[> ---- Array literal ---- <]*/
/*if (islbracket(c)) {*/
/*ctx->bracketLevel++;*/
/*(*position)++;*/
/*while (buffer[*position] && !isrbracket(buffer[*position]))*/
/*(*position)++;*/
/*if (isrbracket(buffer[*position])) {*/
/*ctx->bracketLevel--;*/
/*(*position)++;*/
/*}*/
/*ctx->current = CONTEXT_ARRAY_LITERAL;*/
/*vi->isArray = true;*/
/*return true;*/
/*}*/

/*[> ---- Default fallback ---- <]*/
/*ctx->current = CONTEXT_EXPRESSION;*/
/*return true;*/
/*}*/
