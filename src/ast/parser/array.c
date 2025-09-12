#include <rupa/package.h>

Response parseDeepArray(Request *req, Response res, int start, int end) {
  if (!req)
    return res;

  Token *t = req->tokens;

  if (isToken(t, start, LBLOCK)) {
    int r = findArr(t, start);
    res.rightId = parseArray(req, res, start + 1, r);
    res.nextId = isToken(t, r + 1, COMMA) ? r + 2 : r + 1;
    return res;
  }

  int pos = start;
  while (pos < end && !isToken(t, pos, COMMA)) {
    pos++;
  }

  if (start >= pos)
    return res;

  res.rightId = parseBinary(req, start, pos);
  res.nextId = isToken(t, pos, COMMA) ? pos + 1 : pos;
  return res;
}

int parseArray(Request *req, Response res, int start, int end) {
  if (!req)
    return -1;

  int capacity = 10;
  int *elements = malloc(capacity * sizeof(int));
  if (!elements)
    return -1;

  int length = 0;
  int pos = start;

  while (pos < end) {
    res = parseDeepArray(req, res, pos, end);
    if (res.rightId > 0) {
      // expand kalau perlu
      if (length >= capacity) {
        int newCapacity = capacity * 2;
        int *newElements = realloc(elements, newCapacity * sizeof(int));
        if (!newElements) {
          free(elements);
          return -1;
        }
        elements = newElements;
        capacity = newCapacity;
      }
      elements[length++] = res.rightId;
    }

    if (res.nextId <= pos)
      break; // safety biar nggak infinite loop
    pos = res.nextId;
  }

  return createArray(req->node, elements, length);
}
