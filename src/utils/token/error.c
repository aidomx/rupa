#include <rupa/package.h>

void handleUnexpectedToken(Request *req, DataToken *data, int init) {
  if (req->right.end >= req->tokens->length && !match(data, ENDOF)) {
    fprintf(stderr, "Warning: unexpected token '%s' (type %d) at index %d\n",
            data->value, data->type, init);
  }
}
