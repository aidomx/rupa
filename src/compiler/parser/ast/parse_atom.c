#include <rupa.h>

/**
 * parseAtom: memproses atom (IDENTIFIER atau NUMBER).
 */
int parseAtom(Request *req, DataToken *data) {
  if (!data)
    return -1;

  switch (data->type) {
  case BOOLEAN:
    return createBoolean(req->node,
                         strcmp(data->value, "true") == 0 ? true : false);

  case DECIMAL:
    return createDecimal(req->node, data->value);

  /*
   * IDENTIFIER dan LITERAL_ID mempunyai peran lexer yang berbeda, tetapi
   * ketika sudah masuk jalur expression keduanya berarti "membaca nilai".
   * Jangan mempertahankan perbedaan token tersebut di AST expression.
   *
   * Declaration/target tetap dibuat oleh grammar masing-masing dengan
   * NODE_IDENTIFIER. Contoh:
   *
   *   x = 1              -> target: Identifier(x)
   *   return x + y       -> Literal ID(x) + Literal ID(y)
   *   fullname = first + last
   *                       -> Literal ID(first) + Literal ID(last)
   */
  case IDENTIFIER:
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
