#include <rupa/package.h>

/**
 * @brief Memproses sebuah assignment statement (contoh: x = 1).
 *
 * @param req  Pointer ke Request yang berisi context parsing.
 * @param t    Token stream.
 * @param init Index token awal dari assignment.
 *
 * @return Response yang memuat ID node AST hasil assignment,
 *         serta ID kiri (leftId) dan kanan (rightId).
 *
 * @note Fungsi ini:
 * - Mengecek validitas sisi kiri dan kanan assignment.
 * - Membuat node AST assignment dan menambahkannya ke program.
 * - Menangani error jika tidak valid.
 */
Response processAssignment(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  // src/utils/token/posix.c
  int leftId = getLeftSide(t, init);

  if (leftId >= 0 && !isToken(t, leftId, IDENTIFIER)) {
    return res;
  } else if (leftId == -1) {
    fprintf(stderr, "Error: assignment at index %d has no left-hand side\n",
            init);
    return res;
  }

  int rightId = isToken(t, init + 1, ASSIGN) ? init + 2 : -1;
  if (rightId == -1) {
    fprintf(stderr, "Error: assignment at index %d has no right-hand side\n",
            init);
    return res;
  }

  req->left = leftId;
  req->right.start = rightId;
  req->right.end = lastIndex(t, init);
  res = parseStatement(req, res);

  if (res.leftId == -1 && res.rightId == -1) {
    fprintf(stderr, "Error: invalid assignment expression near index %d\n",
            init);
    return res;
  }

  int nodeId = createAssignment(req->node, res.leftId, res.rightId);

  if (nodeId == -1) {
    fprintf(stderr, "Error: failed to create assignment node at index %d\n",
            init);
    return res;
  }

  addToProgram(req->node, req->programId, nodeId);
  res.nodeId = nodeId;

  return res;
}

/**
 * @brief Memproses ekspresi standalone (contoh: x + y).
 *
 * @param req  Pointer ke Request.
 * @param t    Token stream.
 * @param init Index awal token ekspresi.
 *
 * @return Response berisi node AST untuk ekspresi,
 *         dibungkus sebagai "return statement".
 *
 * @note Digunakan untuk ekspresi yang bukan assignment.
 */
Response processStandaloneExpression(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  int end = lastIndex(t, init);
  int expr_id = parseBinary(req, init, end);

  if (expr_id >= 0) {
    int return_id = createReturn(req->node, expr_id);
    addToProgram(req->node, req->programId, return_id);
    res.nodeId = return_id;
  } else {
    fprintf(stderr, "Error: failed to parse expression starting at index %d\n",
            init);
  }

  return res;
}

/**
 * @brief Dispatcher utama untuk memilih handler berdasarkan jenis token.
 *
 * @param req  Pointer ke Request.
 * @param t    Token stream.
 * @param init Index awal token yang sedang diproses.
 *
 * @return Response hasil pemrosesan token:
 *         - Assignment → processAssignment()
 *         - Standalone expression → processStandaloneExpression()
 *         - Token tak terduga → error handler
 */
Response generateHandler(Request *req, Token *t, int init) {
  Response res = {.leftId = -1, .nodeId = -1, .rightId = -1};

  if (!req || !t || init < 0 || init >= t->length)
    return res;

  if (isAssignmentStatement(t, init)) {
    return processAssignment(req, t, init);
  }

  int standalone = isStandaloneToken(t, init);

  if (standalone >= 0)
    return processStandaloneExpression(req, t, init);

  handleUnexpectedToken(req, getToken(t, init), init);
  return res;
}

/**
 * @brief Loop utama untuk membangun AST dari seluruh token.
 *
 * @param req Pointer ke Request.
 * @return Pointer ke Node (root AST program).
 *
 * @details
 * Fungsi ini akan:
 * - Melewati whitespace (NEWLINE, TAB).
 * - Memanggil generateHandler() untuk setiap token signifikan.
 * - Menyusun node hasil parsing ke dalam AST program.
 *
 * @note Semua node program disimpan di dalam req->node.
 */
Node *processGenerate(Request *req) {
  if (!req || !req->tokens)
    return NULL;

  Token *t = req->tokens;
  int pos = 0;

  // Skip whitespace tokens at the beginning
  while (pos < t->length && !isToken(t, pos, ENDOF)) {
    if (isToken(t, pos, NEWLINE) || isToken(t, pos, TAB)) {
      pos++;
      continue;
    }
    break;
  }

  // Process tokens to build AST
  int i = pos;
  while (i < t->length && !isToken(t, i, ENDOF)) {
    Response res = generateHandler(req, t, i);

    if (res.nodeId >= 0) {
      int end = lastIndex(t, i);
      if (end > i) {
        i = end;
        continue;
      }
    }

    if (isToken(t, i, NEWLINE) || isToken(t, i, TAB)) {
      i++;
      continue;
    }

    i++;
  }

  return req->node;
}
