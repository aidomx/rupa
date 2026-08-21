#include <rupa.h>

int createObject(Node *root, struct AstObjectEntry *entries, int length);

static bool ws(Token *t, int i) {
  return i >= 0 && i < t->length &&
         (t->data[i].type == NEWLINE || t->data[i].type == TAB);
}
static bool val(Token *t, int i, const char *s) {
  return i >= 0 && i < t->length && t->data[i].value &&
         strcmp(t->data[i].value, s) == 0;
}
static int lineEnd(Token *t, int i) {
  if (!t || i >= t->length)
    return i;
  int line = t->data[i].line;
  while (i < t->length && !isToken(t, i, ENDOF) && t->data[i].type != NEWLINE &&
         t->data[i].line == line)
    i++;
  return i;
}
static int args(Request *r, int a, int b, int **out);
static int matchClose(Token *t, int open, int end, TokenType l, TokenType r) {
  int d = 0;
  for (int i = open; i < end; i++) {
    if (t->data[i].type == l)
      d++;
    else if (t->data[i].type == r && --d == 0)
      return i;
  }
  return -1;
}
static int expr(Request *r, int a, int b) {
  while (a < b && ws(r->tokens, a))
    a++;
  while (b > a && ws(r->tokens, b - 1))
    b--;
  if (a >= b)
    return -1;
  Token *t = r->tokens;
  if (t->data[a].type == LBLOCK) {
    int c = matchClose(t, a, b, LBLOCK, RBLOCK);
    if (c == b - 1) {
      int *es = NULL, n = args(r, a + 1, c, &es);
      int id = createArray(r->node, es, n);
      return id;
    }
  }
  if (t->data[a].type == LBRACE) {
    int c = matchClose(t, a, b, LBRACE, RBRACE);
    if (c == b - 1) {
      struct AstObjectEntry *entries = NULL;
      int n = 0, start = a + 1, d = 0;
      for (int i = a + 1; i <= c; i++) {
        if (i < c) {
          TokenType q = t->data[i].type;
          if (q == LPAREN || q == LBLOCK || q == LBRACE)
            d++;
          else if (q == RPAREN || q == RBLOCK || q == RBRACE)
            d--;
        }
        if (i == c || (i < c && t->data[i].type == COMMA && d == 0)) {
          int sep = -1, depth = 0;
          for (int j = start; j < i; j++) {
            TokenType q = t->data[j].type;
            if (q == LPAREN || q == LBLOCK || q == LBRACE)
              depth++;
            else if (q == RPAREN || q == RBLOCK || q == RBRACE)
              depth--;
            else if (q == COLON && depth == 0) {
              sep = j;
              break;
            }
          }
          if (sep >= 0) {
            int key = expr(r, start, sep), value = expr(r, sep + 1, i);
            if (key >= 0 && value >= 0) {
              entries = gcrealloc(entries, sizeof(*entries) * (n + 1));
              entries[n++] = (struct AstObjectEntry){key, value};
            }
          }
          start = i + 1;
        }
      }
      int id = createObject(r->node, entries, n);
      return id;
    }
  }
  if (t->data[a].type == IDENTIFIER || t->data[a].type == LITERAL_ID) {
    if (a + 1 < b && t->data[a + 1].type == LPAREN) {
      int c = matchClose(t, a + 1, b, LPAREN, RPAREN);
      if (c == b - 1) {
        int callee = parseAtom(r, &t->data[a]);
        int *as = NULL, n = args(r, a + 2, c, &as);
        int id = createCall(r->node, callee, as, n);
        return id;
      }
    }
  }
  return parseBinary(r, a, b);
}
static void push(int **v, int *n, int x) {
  *v = gcrealloc(*v, sizeof(int) * (*n + 1));
  (*v)[(*n)++] = x;
}
static int args(Request *r, int a, int b, int **out) {
  int *ids = NULL, n = 0, start = a, d = 0;
  Token *t = r->tokens;
  for (int i = a; i <= b; i++) {
    if (i < b) {
      TokenType q = t->data[i].type;
      if (q == LPAREN || q == LBLOCK || q == LBRACE)
        d++;
      else if (q == RPAREN || q == RBLOCK || q == RBRACE)
        d--;
    }
    if (i == b || (i < b && t->data[i].type == COMMA && d == 0)) {
      int id = expr(r, start, i);
      if (id >= 0)
        push(&ids, &n, id);
      start = i + 1;
    }
  }
  *out = ids;
  return n;
}
static int statement(Request *r, int *pos, int limit);
static int block(Request *r, int open, int close) {
  int *ids = NULL, n = 0, p = open + 1;
  while (p < close) {
    while (p < close && ws(r->tokens, p))
      p++;
    if (p >= close)
      break;
    int id = statement(r, &p, close);
    if (id >= 0)
      push(&ids, &n, id);
    else
      p++;
  }
  int id = createBlock(r->node, ids, n);
  return id;
}
/* Build a keyword body. A ':' body is exactly one physical line; a '{...}'
 * body owns the complete brace range. This prevents the next top-level
 * statement from being accidentally absorbed as part of a single-line body. */
static int keywordBlock(Request *r, int bodyStart, int limit, int *next) {
  Token *t = r->tokens;
  if (next)
    *next = bodyStart;
  if (bodyStart >= limit)
    return -1;

  if (t->data[bodyStart].type == LBRACE) {
    int c = matchClose(t, bodyStart, limit, LBRACE, RBRACE);
    if (c < 0)
      return -1;
    if (next)
      *next = c + 1;
    return block(r, bodyStart, c);
  }

  int end = lineEnd(t, bodyStart);
  if (end > limit)
    end = limit;
  int p = bodyStart, id = statement(r, &p, end);
  if (id < 0)
    return -1;
  if (next)
    *next = end;
  return createBlock(r->node, &id, 1);
}
static int statement(Request *r, int *pos, int limit) {
  Token *t = r->tokens;
  while (*pos < limit && ws(t, *pos))
    (*pos)++;
  if (*pos >= limit)
    return -1;
  int a = *pos, b = lineEnd(t, a);
  if (b > limit)
    b = limit;
  // keywords
  if (t->data[a].type == KEYWORD) {
    const char *k = t->data[a].value;
    if (!strcmp(k, "return")) {
      int e = expr(r, a + 1, b);
      *pos = b;
      return e >= 0 ? createReturn(r->node, e) : -1;
    }
    if (!strcmp(k, "print")) {
      int o = a + 1, c = (o < b && t->data[o].type == LPAREN)
                             ? matchClose(t, o, b, LPAREN, RPAREN)
                             : -1;
      int *as = NULL, n = (c >= 0) ? args(r, o + 1, c, &as) : 0;
      int id = createPrint(r->node, as, n);
      *pos = b;
      return id;
    }
    if (!strcmp(k, "if") || !strcmp(k, "elseif")) {
      int sep = b;
      for (int i = a + 1; i < b; i++)
        if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
          sep = i;
          break;
        }
      int cond = expr(r, a + 1, sep);
      int bodyStart = (sep < b && t->data[sep].type == COLON) ? sep + 1 : sep;
      int next = b;
      int body = keywordBlock(r, bodyStart, limit, &next);
      int elseBlock = -1;
      *pos = next;
      return createIf(r->node, cond, body, elseBlock);
    }
    if (!strcmp(k, "else")) {
      int bs = a + 1;
      if (bs < b && t->data[bs].type == LBRACE) {
      } else if (bs < b && t->data[bs].type == COLON)
        bs++;
      int next = b;
      int body = keywordBlock(r, bs, limit, &next);
      *pos = next;
      return createIf(r->node, -1, body, -1);
    }
    if (!strcmp(k, "for") || !strcmp(k, "rev") || !strcmp(k, "while")) {
      int sep = b;
      for (int i = a + 1; i < b; i++)
        if (t->data[i].type == COLON || t->data[i].type == LBRACE) {
          sep = i;
          break;
        }
      int c = expr(r, a + 1, sep);
      int bs = (sep < b && t->data[sep].type == COLON) ? sep + 1 : sep;
      int next = b;
      int body = keywordBlock(r, bs, limit, &next);
      *pos = next;
      return createLoop(r->node, k, c, body);
    }
    if (!strcmp(k, "import") || !strcmp(k, "export") || !strcmp(k, "extends")) {
      int v = expr(r, a + 1, b);
      NodeType nt = !strcmp(k, "import")
                        ? NODE_IMPORT
                        : (!strcmp(k, "export") ? NODE_EXPORT : NODE_EXTENDS);
      *pos = b;
      return createModule(r->node, nt, v);
    }
  }
  // name(...) { } => function, name(...) => call
  if (t->data[a].type == IDENTIFIER && a + 1 < b &&
      t->data[a + 1].type == LPAREN) {
    int c = matchClose(t, a + 1, limit, LPAREN, RPAREN);
    if (c > 0) {
      int name = parseAtom(r, &t->data[a]);
      int *ps = NULL, n = 0, start = a + 2;
      for (int j = a + 2; j <= c; j++) {
        if (j == c || (j < c && t->data[j].type == COMMA)) {
          int sep = -1;
          for (int q = start; q < j; q++)
            if (t->data[q].type == COLON) {
              sep = q;
              break;
            }
          int pid = -1;
          if (sep >= 0) {
            int pn = expr(r, start, sep), pt = expr(r, sep + 1, j);
            pid = createAnnotation(r->node, pn, pt, -1);
          } else
            pid = expr(r, start, j);
          if (pid >= 0)
            push(&ps, &n, pid);
          start = j + 1;
        }
      }
      if (c + 1 < limit && t->data[c + 1].type == LBRACE) {
        int close = matchClose(t, c + 1, limit, LBRACE, RBRACE);
        int body = close >= 0 ? block(r, c + 1, close) : -1;
        int id = createFunctionDecl(r->node, name, ps, n, body);
        *pos = close >= 0 ? close + 1 : b;
        return id;
      }
      int id = createCall(r->node, name, ps, n);
      *pos = b;
      return id;
    }
  }
  // Name { } => struct/blueprint
  if (t->data[a].type == IDENTIFIER && a + 1 < limit &&
      t->data[a + 1].type == LBRACE) {
    int close = matchClose(t, a + 1, limit, LBRACE, RBRACE);
    if (close >= 0) {
      int name = parseAtom(r, &t->data[a]);
      int body = block(r, a + 1, close);
      *pos = close + 1;
      return createStructDecl(r->node, name, body);
    }
  }
  // annotation name: type [= value]
  if (t->data[a].type == IDENTIFIER && a + 1 < b &&
      t->data[a + 1].type == COLON) {
    int eq = -1;
    for (int i = a + 2; i < b; i++)
      if (t->data[i].type == ASSIGN) {
        eq = i;
        break;
      }
    int name = parseAtom(r, &t->data[a]);
    int type = expr(r, a + 2, eq >= 0 ? eq : b);
    int value = eq >= 0 ? expr(r, eq + 1, b) : -1;
    *pos = b;
    return createAnnotation(r->node, name, type, value);
  }
  // standalone annotation already normalized by lexer: name: Type ->
  // IDENTIFIER(name, safetyType=Type)
  if (t->data[a].type == IDENTIFIER && t->data[a].safetyType &&
      (a + 1 >= b || t->data[a + 1].type != ASSIGN)) {
    int name = parseAtom(r, &t->data[a]);
    int type = createId(r->node, t->data[a].safetyType);
    *pos = a + 1;
    return createAnnotation(r->node, name, type, -1);
  }
  // assignment
  for (int i = a + 1; i < b; i++)
    if (t->data[i].type == ASSIGN) {
      int l = expr(r, a, i), rr = expr(r, i + 1, b);
      int type = -1;

      if (t->data[a].type == IDENTIFIER && t->data[a].safetyType)
        type = createId(r->node, t->data[a].safetyType);

      *pos = b;
      if (l >= 0 && rr >= 0)
        return createAssignment(r->node, l, type, rr);
      return -1;
    }
  int e = expr(r, a, b);
  *pos = b;
  return e >= 0 ? createReturn(r->node, e) : -1;
}

Response processAssignment(Request *req, Token *t, int init) {
  Response z = {-1, -1, -1, -1};
  int p = init, id = statement(req, &p, t->length);
  z.nodeId = id;
  return z;
}
Response processStandaloneExpression(Request *req, Token *t, int init) {
  return processAssignment(req, t, init);
}
Response generateHandler(Request *req, Token *t, int init) {
  return processAssignment(req, t, init);
}
Node *processGenerate(Request *req) {
  if (!req || !req->tokens)
    return NULL;
  int p = 0;
  while (p < req->tokens->length && !isToken(req->tokens, p, ENDOF)) {
    while (p < req->tokens->length && ws(req->tokens, p))
      p++;
    if (p >= req->tokens->length || isToken(req->tokens, p, ENDOF))
      break;
    int before = p, id = statement(req, &p, req->tokens->length);
    if (id >= 0)
      addToProgram(req->node, req->programId, id);
    if (p <= before)
      p++;
  }
  return req->node;
}
