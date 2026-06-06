#include <rupa.h>

/*static void *isExpressionValidation(Input *input) {*/
/*if (!input || !input->buffer || !input->context || input->cursor == -1 ||*/
/*!input->validation)*/
/*return NULL;*/

/*Context *ctx = input->context;*/
/*ValidationInput *vi = input->validation;*/
/*const char *buffer = input->content;*/
/*int posId = input->cursor;*/

/*if (!vi->isIdentifier) {*/
/*vi->isIdentifier = getIdentifier(buffer, &posId) != -1;*/
/*}*/

/*int nextId = posId;*/
/*if (buffer[nextId] == '\0') {*/
/*ctx->current = CONTEXT_VAR_DECLARATION;*/
/*vi->isComplete = true;*/
/*}*/

/*else {*/
/*ctx->current = CONTEXT_IDENTIFIER_REFERENCE;*/
/*setNextProgram(buffer, ctx, &nextId, vi);*/
/*}*/
/*input->cursor = nextId;*/
/*return vi;*/
/*}*/

/**
 * @brief Proses validasi format keyword berdasarkan konteks.
 * Setiap jenis keyword memiliki pola validasi masing-masing.
 */
/*static void *isKeywordValidation(Input *input) {*/
/*if (!input || !input->keyword || !input->buffer)*/
/*return NULL;*/

/*Context *ctx = input->context;*/
/*Keyword *k = input->keyword;*/
/*ValidationInput *vi = input->validation;*/
/*const char *buffer = input->content;*/
/*int start = input->cursor;*/

/*switch (k->type) {*/
/*case KEYWORD_IF:*/
/*vi->isBlockIf = k->insideIf && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_IF_STATEMENT;*/
/*break;*/

/*case KEYWORD_ELSEIF:*/
/*vi->isBlockElseIf = k->insideIf && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_ELSEIF_STATEMENT;*/
/*break;*/

/*case KEYWORD_ELSE:*/
/*vi->isBlockElse = k->insideIf && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_ELSE_STATEMENT;*/
/*break;*/

/*case KEYWORD_FOR:*/
/*vi->isFor = k->insideLoop && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_FOR_LOOP;*/
/*break;*/

/*case KEYWORD_REV:*/
/*vi->isRev = k->insideLoop && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_REV_LOOP;*/
/*break;*/

/*case KEYWORD_WHILE:*/
/*vi->isWhile = k->insideLoop && isValidBlock(buffer, &start);*/
/*ctx->current = CONTEXT_WHILE_LOOP;*/
/*break;*/

/*case KEYWORD_PRINT:*/
/*vi->isPrint = isValidPrint(buffer, &start);*/
/*if (!vi->isPrint)*/
/*return vi;*/
/*start = consumeToEnd(buffer, start);*/
/*if (buffer[start] == '\0') {*/
/*ctx->current = CONTEXT_PRINT_STATEMENT;*/
/*vi->isComplete = true;*/
/*}*/

/*break;*/

/*case KEYWORD_EXTENDS:*/
/*if (!k->insideIf && !k->insideLoop) {*/
/*vi->isExtends = isValidModule(buffer, &start);*/
/*ctx->current = CONTEXT_EXTENDS_DECLARATION;*/
/*}*/
/*break;*/

/*case KEYWORD_IMPORT:*/
/*if (!k->insideIf && !k->insideLoop) {*/
/*vi->isImport = isValidModule(buffer, &start);*/
/*ctx->current = CONTEXT_IMPORT_DECLARATION;*/
/*}*/
/*break;*/

/*case KEYWORD_EXPORT:*/
/*if (!k->insideIf && !k->insideLoop) {*/
/*vi->isExport = isValidModule(buffer, &start);*/
/*ctx->current = CONTEXT_EXPORT_DECLARATION;*/
/*}*/
/*break;*/

/*case KEYWORD_RETURN:*/
/*if (!k->insideIf && !k->insideLoop) {*/
/*vi->isReturn =*/
/*isValidBlock(buffer, &start) || isValidModule(buffer, &start);*/
/*ctx->current = CONTEXT_RETURN_STATEMENT;*/
/*}*/
/*break;*/

/*default:*/
/*ctx->current = CONTEXT_EXPRESSION;*/
/*break;*/
/*}*/

/*input->cursor = start;*/
/*return vi;*/
/*}*/

/**
 * @brief Fungsi utama untuk memproses validasi input.
 * Ini bertugas membaca keyword dan memeriksa format setelahnya.
 */
/*void *processValidationInput(Input *input) {*/
/*if (!input || !input->validation)*/
/*return NULL;*/

/*return !getKeyword(input) ? isExpressionValidation(input)*/
/*: isKeywordValidation(input);*/

/*Context *ctx = input->context;*/
/*Keyword *keyword = getKeyword(input);*/
/*ctx->prev = ctx->current;*/

/*if (keyword && keyword->type != KEYWORD_NONE) {*/
/*isKeywordValidation(input);*/
/*} else {*/
/*isProgramValidation(input);*/
/*}*/

/*printf("Context:\n");*/
/*printf("- prev: %d\n", ctx->prev);*/
/*printf("- current: %d\n", ctx->current);*/
// Reset cursor untuk menjaga state konsisten
// input->cursor = 0;
/*}*/
