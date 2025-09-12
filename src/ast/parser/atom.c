#include <rupa/package.h>

/**
 * parseAtom: memproses atom (IDENTIFIER atau NUMBER).
 */
int parseAtom(Request *req, DataToken *data) {
  if (!data)
    return -1;

  Token *t = req->tokens;
  int pos = (int)(data - t->data);

  switch (data->type) {
  case BOOLEAN:
    return createBoolean(req->node,
                         strcmp(data->value, "true") == 0 ? true : false);

  case FLOAT:
    return createFloat(req->node, data->value);

  case IDENTIFIER: {
    int baseId = createId(req->node, data->value, data->safetyType);

    // Periksa jika identifier diikuti oleh LBLOCK (array access)
    if (pos + 1 < t->length && match(&t->data[pos + 1], LBLOCK)) {
      baseId = parseSubscripts(req, baseId, pos + 1);
    }

    return baseId;
  };

  case LITERAL_ID:
    return createString(req->node, data->value, NODE_LITERAL_ID);

  case NUMBER:
    return createNumber(req->node, atoi(data->value));

  case NULLABLE:
    return createString(req->node, data->value, NODE_NULLABLE);

  case STRING:
    return createString(req->node, data->value, NODE_STRING);

  default:
    return -1;
  }
}
