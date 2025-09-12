#include <rupa/package.h>

/**
 * parseFactor: memproses faktor kiri assignment (IDENTIFIER).
 */
int parseFactor(Request *req, Response res) {
  if (req->left == -1 || req->right.start >= req->right.end)
    return -1;

  Token *t = req->tokens;
  int pos = req->left;
  DataToken *data = &t->data[pos];
  int baseId = res.leftId;

  if (match(data, IDENTIFIER)) {
    baseId = createId(req->node, data->value, data->safetyType);

    // Periksa jika ada subscript (array access)
    if (pos + 1 < t->length && match(&t->data[pos + 1], LBLOCK)) {
      baseId = parseSubscripts(req, baseId, pos + 1);
    }

    return baseId;
  }

  // Handle other cases if needed
  return baseId;
}
