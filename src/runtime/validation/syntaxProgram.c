#include <rupa.h>

/*bool isNextIdentifier(char c) {*/
/*return isassign(c) || islbracket(c) || islblock(c) || iscolon(c) ||*/
/*islparen(c);*/
/*}*/

/*void resetValidate(ValidationInput *vi) {*/
/*vi->isArray = false;*/
/*vi->isAssignment = false;*/
/*vi->isAnnotionType = false;*/
/*vi->isIdentifier = false;*/
/*vi->isFunction = false;*/
/*vi->isStructDecl = false;*/

/*if (vi->isComplete) {*/
/*vi->isComplete = false;*/
/*vi->isWaiting = false;*/
/*}*/
/*}*/

/*int validateNextProgram(const char *buffer, Context *ctx, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return -1;*/

/*return (*position);*/
/*}*/

/*void setNextProgram(const char *buffer, Context *ctx, int *position,*/
/*ValidationInput *vi) {*/
/*if (!buffer || !ctx || *position == -1 || !vi)*/
/*return;*/

/*if (!vi->isIdentifier)*/
/*return;*/

/*int nextId = (*position);*/
/*skipWhitespace(buffer, &nextId);*/

/*switch (buffer[nextId]) {*/
/*case '=':*/
/*printf("assignment\n");*/
/*break;*/

/*case ':':*/
/*printf("annotion type\n");*/
/*break;*/

/*case '[':*/
/*case ']':*/
/*printf("array\n");*/
/*break;*/

/*case '(':*/
/*case ')':*/
/*nextId = parenthesis(buffer, nextId, &ctx->parenLevel);*/
/*ctx->current = CONTEXT_FUNCTION_CALL;*/
/*vi->isFunction = true;*/
/*vi->isWaiting = true;*/
/*break;*/

/*case '{':*/
/*case '}':*/
/*nextId = braces(buffer, nextId, &ctx->braceLevel);*/
/*ctx->current = !vi->isFunction ? CONTEXT_STRUCT_DECLARATION*/
/*: CONTEXT_FUNCTION_DECLARATION;*/
/*vi->isWaiting = ctx->braceLevel == 0 ? false : true;*/
/*break;*/

/*case '\0':*/
/*default:*/
/*ctx->current = CONTEXT_VAR_DECLARATION;*/
/*vi->isWaiting = false;*/
/*break;*/
/*}*/

/*// match: uncomplete function*/
/*[>if (vi->isFunction && vi->isWaiting) {<]*/
/*[>*position = nextId;<]*/
/*[>vi->isFunction = false;<]*/
/*[>vi->isWaiting = false;<]*/
/*[>return;<]*/
/*[>}<]*/

/*// skip: [space]*/
/*// match: identifier only*/
/*[>if (!isNextIdentifier(buffer[nextId]) && buffer[nextId] != '\0') {<]*/
/*[>nextId = consumeToEnd(buffer, nextId);<]*/

/*[>if (buffer[nextId] == '\0') {<]*/
/*[>ctx->current = CONTEXT_VAR_DECLARATION;<]*/
/*[>vi->isWaiting = false;<]*/
/*[>}<]*/
/*[>}<]*/

/*vi->isComplete = !vi->isWaiting;*/
/**position = nextId;*/
/*}*/
