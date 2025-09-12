#include <rupa/package.h>

int parseSubscripts(Request *req, int baseId, int start) {
  Token *t = req->tokens;
  int pos = start;
  int currentId = baseId;

  while (pos < t->length && isToken(t, pos, LBLOCK)) {
    int rpos = findArr(t, pos);
    if (rpos == -1)
      break;

    int index;
    if (rpos == (pos + 1)) {
      index = -1;
    } else {
      index = parseBinary(req, pos + 1, rpos);
    }

    currentId = createSubscript(req->node, currentId, index);

    AstNode *node = &req->node->ast[index];
    if (index >= 0 && node->type == NODE_SUBSCRIPT) {
      currentId = parseSubscripts(req, currentId, pos + 1);
    }

    pos = rpos + 1;
  }

  return currentId;
}
