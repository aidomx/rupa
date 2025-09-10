#include <rupa/package.h>

static int parseArray(Request *req, Response res, int start, int end);

int parseSubscripts(Request *req, int baseId, int start) {
  Token *t = req->tokens;
  int pos = start;
  int currentId = baseId;

  while (pos < t->length && match(&t->data[pos], LBLOCK)) {
    int rblock_pos = findArr(t, pos);
    if (rblock_pos == -1)
      break;

    if (rblock_pos == pos + 1) { // Empty brackets []
      currentId = createSubscript(req->node, currentId, -1);
    } else {
      int index = parseBinary(req, pos + 1, rblock_pos);
      currentId = createSubscript(req->node, currentId, index);
    }

    pos = rblock_pos + 1;
  }

  return currentId;
}

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

/**
 * parseBinary: parser rekursif untuk binary expression.
 * - Menjaga precedence.
 * - Mengabaikan operator dalam tanda kurung.
 */
int parseBinary(Request *req, int start, int end) {
  if (!req || start >= end)
    return -1;
  Token *tokens = req->tokens;

  int minPrec = 0x3f3f3f3f;
  int minIndex = -1;
  int depth = 0;

  // Skip whitespace at beginning
  while (start < end && (match(&tokens->data[start], NEWLINE) ||
                         match(&tokens->data[start], TAB))) {
    start++;
  }

  // Skip whitespace at end
  while (end > start && (match(&tokens->data[end - 1], NEWLINE) ||
                         match(&tokens->data[end - 1], TAB))) {
    end--;
  }

  if (start >= end)
    return -1; // Empty after trimming

  // cari operator top-level (depth == 0)
  for (int i = start; i < end; i++) {
    if (match(&tokens->data[i], LPAREN) || match(&tokens->data[i], LBLOCK) ||
        match(&tokens->data[i], COMMA)) {
      depth++;
      continue;
    }
    if (match(&tokens->data[i], RPAREN) || match(&tokens->data[i], RBLOCK)) {
      depth--;
      continue;
    }

    if (depth > 0)
      continue;

    int prec = getPrecedence(&tokens->data[i]);
    if (prec >= 0 && prec <= minPrec) {
      minPrec = prec;
      minIndex = i;
    }
  }

  // tidak ada operator di level atas
  if (minIndex == -1) {
    if (match(&tokens->data[start], LPAREN)) {
      int k = findParen(tokens, start, end);
      if (k == end - 1) {
        // kupas kurung luar
        return parseBinary(req, start + 1, k);
      }
    }

    else if (match(&tokens->data[start], LBLOCK)) {
      int k = findArr(tokens, start + 1);
      if (k == end - 1) {
        return parseBinary(req, start + 1, k);
      }
    }

    return parseAtom(req, &tokens->data[start]);
  }

  // pecah kiri dan kanan
  int left = parseBinary(req, start, minIndex);
  int right = parseBinary(req, minIndex + 1, end);

  return createBinary(req->node, &tokens->data[minIndex], left, right);
}

/**
 * parseExpression: memproses ekspresi kanan assignment.
 */
Response parseExpression(Request *req, Response res) {
  if (req->right.start >= req->right.end)
    return res;

  Token *t = req->tokens;
  int start = req->right.start;
  int end = req->right.end;

  // Handle expressions in parentheses
  if (match(&t->data[start], LPAREN)) {
    int rparen_pos = findParen(t, start, end);
    if (rparen_pos != -1) {
      // Parse the expression inside parentheses
      res.rightId = parseBinary(req, start + 1, rparen_pos);
      return res;
    }
  }

  else if (match(&t->data[start], LBLOCK)) {
    res.rightId = parseArray(req, res, start + 1, end);
    return res;
  }

  res.rightId = parseBinary(req, start, end);
  return res;
}

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

/**
 * parseStatement: memproses 1 statement.
 * grammar: identifier = expression
 */
Response parseStatement(Request *req, Response res) {
  if (req->left == -1)
    return parseExpression(req, res);
  res.leftId = parseFactor(req, res);
  return parseExpression(req, res);
}
