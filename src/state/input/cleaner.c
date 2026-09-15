#include <rupa.h>

void clearInput(Input *input) {
  if (!input)
    return;

  /*if (input->buffer) {*/
  /*input->buffer[0] = '\0';*/
  /*}*/

  if (input->content) {
    // free(input->value);
    input->content[0] = '\0';
  }

  if (input->flags) {
    memset(input->flags, 0, sizeof(Flags));
  }

  /*if (input->context) {*/
  /*Context *ctx = input->context;*/
  /*ctx->braceLevel = 0;*/
  /*ctx->bracketLevel = 0;*/
  /*ctx->parenLevel = 0;*/
  /*ctx->quoteChar = 0;*/
  /*ctx->inQuotes = 0;*/
  /*ctx->current = CONTEXT_UNKNOWN;*/
  /*ctx->prev = ctx->current;*/
  /*}*/

  if (input->keyword) {
    Keyword *keyword = input->keyword;
    keyword->insideIf = false;
    keyword->insideLoop = false;
    keyword->type = KEYWORD_NONE;
    keyword->length = 0;
  }

  /*if (input->validation) {*/
  /*memset(input->validation, 0, sizeof(ValidationInput));*/
  /*}*/

  input->capacity = 0;
  input->cursor = 0;
  input->line = 0;
  input->row = input->cursor;
  input->length = 0;

  if (!input->next)
    return;

  clearInput(input->next);
  input->next = NULL;
  // free(input);
}
