#include <rupa.h>

Context *createContext(size_t size) {
  Context *ctx = gcmall(size);
  ctx->current = CONTEXT_UNKNOWN;
  ctx->prev = CONTEXT_UNKNOWN;
  ctx->braceLevel = 0;
  ctx->bracketLevel = 0;
  ctx->parenLevel = 0;
  ctx->inQuotes = 0;
  ctx->quoteChar = 0;
  return ctx;
}

StateContext *createStateContext(int capacity) {
  StateContext *ctx = gcmall(capacity * sizeof(StateContext));

  ctx->flagStatus = FLAG_NONE;
  ctx->brace = 0;
  ctx->inStruct = 0;
  ctx->objectDepth = 0;
  ctx->bracket = 0;
  ctx->paren = 0;
  ctx->line = 0;
  ctx->multiline = false;
  ctx->row = 0;

  return ctx;
}
