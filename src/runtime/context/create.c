#include <rupa.h>

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
