#include <rupa.h>

/**
 * Parse expression source code into an AST node using a temporary
 * lexer/parser scope. Used for string interpolation {{ expr }}.
 * Returns node id or -1 on failure.
 */
static int parseInterpExpr(const char *exprSrc, Request *req) {
  if (!exprSrc || !*exprSrc)
    return -1;

  State *state = createGlobalState(8, false);
  if (!state || !state->repl || !state->repl->buffer)
    return -1;

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->repl->buffer;
  size_t len = strlen(exprSrc);
  if ((int)len >= buffer->capacity)
    return -1;

  memcpy(buffer->value, exprSrc, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  if (!addToHistory(state) || !addToInput(state))
    return -1;

  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0)
    return -1;

  /* Parse the expression into a temporary AST */
  Request tmpReq = createRequest(tokens, 8);
  int end = grammarLineEnd(tokens, 0);
  int exprId = grammarParseExpr(&tmpReq, 0, end);
  if (exprId < 0)
    return -1;

  /* Copy temporary AST nodes into the real node pool.
   * We remap node IDs from the temporary pool to the real pool. */
  Node *tmpNode = tmpReq.node;
  int *map = gcmall(sizeof(int) * tmpNode->length);
  if (!map) return -1;
  for (int i = 0; i < tmpNode->length; i++)
    map[i] = -1;

  for (int i = 0; i < tmpNode->length; i++) {
    AstNode *src = &tmpNode->ast[i];
    AstNode n = *src;
    /* Remap child references based on node type */
    switch (n.type) {
    case NODE_BINARY:
      if (n.binary.left >= 0 && n.binary.left < tmpNode->length)
        n.binary.left = map[n.binary.left];
      if (n.binary.right >= 0 && n.binary.right < tmpNode->length)
        n.binary.right = map[n.binary.right];
      break;
    case NODE_CALL: {
      if (n.call.callee >= 0 && n.call.callee < tmpNode->length)
        n.call.callee = map[n.call.callee];
      if (n.call.length > 0 && n.call.args) {
        int *newArgs = gcmall(sizeof(int) * n.call.length);
        for (int j = 0; j < n.call.length; j++) {
          int old = n.call.args[j];
          newArgs[j] = (old >= 0 && old < tmpNode->length) ? map[old] : -1;
        }
        n.call.args = newArgs;
      }
      break;
    }
    case NODE_MEMBER:
      if (n.member.object >= 0 && n.member.object < tmpNode->length)
        n.member.object = map[n.member.object];
      if (n.member.member >= 0 && n.member.member < tmpNode->length)
        n.member.member = map[n.member.member];
      break;
    case NODE_SUBSCRIPT:
      if (n.subscript.posId >= 0 && n.subscript.posId < tmpNode->length)
        n.subscript.posId = map[n.subscript.posId];
      if (n.subscript.index >= 0 && n.subscript.index < tmpNode->length)
        n.subscript.index = map[n.subscript.index];
      break;
    default:
      break;
    }
    map[i] = createAst(req->node, n);
  }

  return map[exprId];
}

/**
 * Try to parse a string token as string interpolation.
 * Detects {{ expr }} (expression) and { name } (variable) patterns.
 * Returns NODE_STRING_INTERP node id, or -1 if no interpolation found.
 */
static int tryParseStringInterp(Request *req, DataToken *data) {
  if (!data || data->type != STRING)
    return -1;

  /* Get the raw string value (with quotes) */
  const char *raw = data->value;
  if (!raw) return -1;

  size_t rawLen = strlen(raw);
  if (rawLen < 2) return -1;

  /* Strip surrounding quotes */
  const char *inner = raw + 1;
  int innerLen = (int)rawLen - 2;
  if (innerLen <= 0) return -1;

  /* Quick check: does it contain any interpolation markers? */
  bool hasInterp = false;
  for (int i = 0; i < innerLen; i++) {
    if (inner[i] == '{') {
      hasInterp = true;
      break;
    }
  }
  if (!hasInterp)
    return -1;

  /* Scan through the string and build parts list */
  int *parts = NULL;
  int partCount = 0;
  int textStart = 0;

  #define PUSH_PART(id) do { \
    parts = gcresize(parts, sizeof(int) * partCount, sizeof(int) * (partCount + 1)); \
    parts[partCount++] = (id); \
  } while(0)

  int i = 0;
  while (i < innerLen) {
    /* Skip escape sequences like \{ or \n */
    if (inner[i] == '\\' && i + 1 < innerLen) {
      i += 2;
      continue;
    }

    /* {{ expr }} : double brace = expression */
    if (inner[i] == '{' && i + 1 < innerLen && inner[i + 1] == '{') {
      /* Push any preceding text as string literal */
      if (textStart < i) {
        int tlen = i - textStart;
        char *buf = gcmall(tlen + 3); /* +2 for quotes +1 for null */
        if (!buf) return -1;
        buf[0] = '"';
        memcpy(buf + 1, inner + textStart, tlen);
        buf[tlen + 1] = '"';
        buf[tlen + 2] = '\0';
        int id = createString(req->node, buf, NODE_STRING);
        gcfree(buf);
        PUSH_PART(id);
      }

      /* Find matching }} */
      const char *close = strstr(inner + i + 2, "}}");
      if (!close || close - inner >= innerLen) {
        /* No matching }}: treat rest as literal text */
        if (textStart < innerLen) {
          int tlen = innerLen - textStart;
          char *buf = gcmall(tlen + 3);
          if (!buf) return -1;
          buf[0] = '"';
          memcpy(buf + 1, inner + textStart, tlen);
          buf[tlen + 1] = '"';
          buf[tlen + 2] = '\0';
          int id = createString(req->node, buf, NODE_STRING);
          gcfree(buf);
          PUSH_PART(id);
        }
        goto done;
      }

      /* Extract expression and parse it */
      int exprLen = (int)(close - inner - i - 2);
      char *exprBuf = gcmall(exprLen + 1);
      if (!exprBuf) return -1;
      memcpy(exprBuf, inner + i + 2, exprLen);
      exprBuf[exprLen] = '\0';

      int exprId = parseInterpExpr(exprBuf, req);
      if (exprId >= 0) {
        gcfree(exprBuf);
        PUSH_PART(exprId);
      } else {
        /* Failed to parse: keep as string literal */
        int litLen = exprLen + 4; /* {{ + content + }} */
        char *litBuf = gcmall(litLen + 1);
        if (!litBuf) { gcfree(exprBuf); return -1; }
        litBuf[0] = '"';
        litBuf[1] = '{';
        litBuf[2] = '{';
        memcpy(litBuf + 3, exprBuf, exprLen);
        litBuf[exprLen + 3] = '}';
        litBuf[exprLen + 4] = '"';
        litBuf[exprLen + 5] = '\0';
        gcfree(exprBuf);
        int id = createString(req->node, litBuf, NODE_STRING);
        gcfree(litBuf);
        PUSH_PART(id);
      }

      i = (int)(close - inner) + 2;
      textStart = i;
      continue;
    }

    /* { name } : single brace = variable lookup */
    if (inner[i] == '{') {
      /* Push any preceding text */
      if (textStart < i) {
        int tlen = i - textStart;
        char *buf = gcmall(tlen + 3);
        if (!buf) return -1;
        buf[0] = '"';
        memcpy(buf + 1, inner + textStart, tlen);
        buf[tlen + 1] = '"';
        buf[tlen + 2] = '\0';
        int id = createString(req->node, buf, NODE_STRING);
        gcfree(buf);
        PUSH_PART(id);
      }

      /* Find matching } */
      const char *close = strchr(inner + i + 1, '}');
      if (!close || close - inner >= innerLen) {
        /* No matching }: treat rest as literal */
        int tlen = innerLen - i;
        char *buf = gcmall(tlen + 3);
        if (!buf) return -1;
        buf[0] = '"';
        memcpy(buf + 1, inner + i, tlen);
        buf[tlen + 1] = '"';
        buf[tlen + 2] = '\0';
        int id = createString(req->node, buf, NODE_STRING);
        gcfree(buf);
        PUSH_PART(id);
        goto done;
      }

      /* Extract variable name and create identifier node */
      int nameLen = (int)(close - inner - i - 1);
      char *nameBuf = gcmall(nameLen + 1);
      if (!nameBuf) return -1;
      memcpy(nameBuf, inner + i + 1, nameLen);
      nameBuf[nameLen] = '\0';

      int id = createId(req->node, nameBuf);
      if (id >= 0) {
        gcfree(nameBuf);
        PUSH_PART(id);
      } else {
        /* Not a valid identifier: keep as literal */
        int tlen = nameLen + 2; /* { + name + } */
        char *buf = gcmall(tlen + 3);
        if (!buf) { gcfree(nameBuf); return -1; }
        buf[0] = '"';
        buf[1] = '{';
        memcpy(buf + 2, nameBuf, nameLen);
        buf[nameLen + 2] = '}';
        buf[nameLen + 3] = '"';
        buf[nameLen + 4] = '\0';
        gcfree(nameBuf);
        int sid = createString(req->node, buf, NODE_STRING);
        gcfree(buf);
        PUSH_PART(sid);
      }

      i = (int)(close - inner) + 1;
      textStart = i;
      continue;
    }

    i++;
  }

  /* Push any remaining text */
  if (textStart < innerLen) {
    int tlen = innerLen - textStart;
    char *buf = gcmall(tlen + 3);
    if (!buf) return -1;
    buf[0] = '"';
    memcpy(buf + 1, inner + textStart, tlen);
    buf[tlen + 1] = '"';
    buf[tlen + 2] = '\0';
    int id = createString(req->node, buf, NODE_STRING);
    gcfree(buf);
    PUSH_PART(id);
  }

#undef PUSH_PART

done:
  if (partCount == 0)
    return -1;

  return createStringInterp(req->node, parts, partCount);
}

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

  case STRING: {
    /* Try to parse as string interpolation first */
    int interpId = tryParseStringInterp(req, data);
    if (interpId >= 0)
      return interpId;
    return createString(req->node, data->value, NODE_STRING);
  }

  default:
    return -1;
  }
}
