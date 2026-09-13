#include <rupa.h>

/*
 * Rupa Code Formatter
 *
 * Walks the AST and outputs properly formatted code.
 * Usage: rupa fmt <file.rp>
 *
 * - Preserves comments (//, block, #)
 * - Preserves blank lines outside blocks
 * - 2-space indentation inside blocks
 * - Opening brace on same line
 *
 * Modular structure:
 * - format_helpers.c: shared helpers (fmtIndent, fmtStr, fmtChar, etc.)
 * - format_node.c: basic node formatters (identifier, literal, number, etc.)
 * - format_expr.c: expression formatters (binary, call, print, array, etc.)
 * - format_stmt.c: statement formatters (assign, if, loop, function, etc.)
 * - format_dispatch.c: main fmtNode dispatch switch
 * - format_comment.c: comment formatting
 * - formatter.c: entry points and source-based formatting (this file)
 */

/* ==================== Source comment scanner ==================== */

/*
 * Skip over a comment at position *p in source[0..end).
 * Returns true and advances *p past the comment if found.
 * Returns false if *p is not at a comment start.
 */

/* ==================== Source-based formatting ==================== */

/*
 * Extract first meaningful keyword/text from an AST node for source matching.
 */
static const char *getDeclNeedle(Node *node, int nodeId) {
  if (nodeId < 0 || nodeId >= node->length) return NULL;
  AstNode *n = &node->ast[nodeId];
  switch (n->type) {
  case NODE_IF:
    return "if";
  case NODE_LOOP:
    return n->loop.kind;
  case NODE_FUNCTION_DECL:
    return getDeclNeedle(node, n->function.name);
  case NODE_STRUCT_DECL:
    return getDeclNeedle(node, n->asStruct.name);
  case NODE_MOD:
    return n->mod.type == ImportDecl ? "import" : "export";
  case NODE_EXTENDS:
    return "extends";
  case NODE_ASSIGN:
    return getDeclNeedle(node, n->assign.target);
  case NODE_CONDITIONAL_ASSIGN:
    return getDeclNeedle(node, n->conditionalAssign.target);
  case NODE_ANNOTATION:
    return getDeclNeedle(node, n->annotation.name);
  case NODE_PRINT:
    return "print";
  case NODE_CASE:
    return "case";
  case NODE_ASYNC:
    return "async";
  case NODE_IDENTIFIER:
    return n->identifier.name;
  case NODE_LITERAL_ID:
    return n->string.value;
  case NODE_MEMBER:
    return getDeclNeedle(node, n->member.object);
  default:
    return NULL;
  }
}

/*
 * Find the start of the next declaration in source.
 *
 * Prefers an occurrence of needle that begins a physical line (only
 * whitespace before it on that line) so a short identifier like `i`
 * does not match inside an unconsumed `for i < x.length` header or
 * `print(...)` body — which used to reorder `i = 0` ahead of the
 * comment preceding it. Falls back to the first plain occurrence so
 * declarations that do not start a line still match.
 */
static int findDeclStart(const char *src, int srcPos, int srcLen,
                         const char *needle) {
  if (!needle) return -1;
  size_t nlen = strlen(needle);
  if (nlen == 0) return -1;

  int pos = srcPos;
  int fallback = -1;
  while (pos + (int)nlen <= srcLen) {
    const char *found = strstr(src + pos, needle);
    if (!found) break;
    int p = (int)(found - src);
    if (fallback < 0) fallback = p;

    /* Walk back over indentation; the match is at line start when the
     * preceding char is a newline (or the match begins the file). */
    int q = p;
    while (q > 0 && src[q - 1] != '\n' &&
           (src[q - 1] == ' ' || src[q - 1] == '\t'))
      q--;
    if (q == 0 || src[q - 1] == '\n')
      return p;

    pos = p + 1;
  }
  return fallback;
}

static int runFormat(State *state, Formatter *fmt) {
  Buffer *buffer = state->buffer;
  const char *src = buffer->value;
  int srcLen = buffer->length;

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (state->input->flags && state->input->flags->isWaiting)) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "LexerError", "no tokens produced",
                       state->input->content, state->input->cursor, ERR_UNEXPECTED_EOF);
    printErrors(state->error);
    return 1;
  }

  Request req = createRequestWithError(tokens, 10, state->error);
  Node *node = processGenerate(&req);
  if (!node || node->length <= 0) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "ParserError", "failed to build AST",
                       state->input->content, state->input->cursor, ERR_SYNTAX);
    printErrors(state->error);
    return 1;
  }

  int root = -1;
  for (int i = 0; i < node->length; i++) {
    if (node->ast[i].type == NODE_PROGRAM) {
      root = i;
      break;
    }
  }

  if (root < 0) {
    addSourceErrorAt(state->error, "ParserError", "program root not found",
                     state->input->content, state->input->cursor, ERR_SYNTAX);
    printErrors(state->error);
    return 1;
  }

  AstNode *prog = &node->ast[root];
  AstDeclaration *current = prog->program.declarations;
  if (!current) return 0;

/* Collect declarations */
#define MAX_DECLS 256
  int declIds[MAX_DECLS];
  int declCount = 0;
  {
    AstDeclaration *d = current;
    while (d && declCount < MAX_DECLS) {
      declIds[declCount] = d->nodeId;
      declCount++;
      d = d->next;
    }
  }

  /*
   * Main formatting loop: walk source text linearly, output comments inline,
   * and output formatted AST nodes at each declaration boundary.
   */
  int srcPos = 0;

  for (int d = 0; d < declCount; d++) {
    /* Find declaration start in source */
    const char *needle = getDeclNeedle(node, declIds[d]);
    int declStart = -1;
    if (needle) {
      declStart = findDeclStart(src, srcPos, srcLen, needle);
    }
    /* For comment declarations without a needle, find the next non-comment
     * declaration's needle so we only scan source up to that point. */
    if (declStart < 0) {
      for (int j = d + 1; j < declCount; j++) {
        AstNode *jn = &node->ast[declIds[j]];
        if (jn->type == NODE_COMMENT || jn->type == NODE_INLINE_COMMENT ||
            jn->type == NODE_BLOCK_COMMENT)
          continue;
        const char *jnNeedle = getDeclNeedle(node, declIds[j]);
        if (jnNeedle) {
          declStart = findDeclStart(src, srcPos, srcLen, jnNeedle);
        }
        break;
      }
    }
    if (declStart < 0) declStart = srcLen; /* fallback: no more source */

    /* Output comments and blank lines between srcPos and declStart */
    while (srcPos < declStart) {
      char c = src[srcPos];
      char next = (srcPos + 1 < srcLen) ? src[srcPos + 1] : 0;

      /* Skip whitespace (but track blank lines) */
      if (c == ' ' || c == '\t' || c == '\r') {
        srcPos++;
        continue;
      }

      /* Preserve blank lines */
      if (c == '\n') {
        /* Check if next non-space char is also \n (blank line) */
        int peek = srcPos + 1;
        while (peek < srcLen && (src[peek] == ' ' || src[peek] == '\t'))
          peek++;
        if (peek < srcLen && src[peek] == '\n') {
          /* Blank line — output it and skip */
          fmtNewline(fmt);
          srcPos = peek + 1;
          continue;
        }
        /* Regular newline — skip */
        srcPos++;
        continue;
      }

      /* Found a comment — format and output it */
      if (c == '#' || (c == '/' && (next == '/' || next == '*'))) {
        formatComment(src, &srcPos, srcLen);
        continue;
      }

      /* Found code — skip it (part of previous declaration) */
      srcPos++;
    }

    /* Skip comment declarations - already output by source scanning */
    AstNode *declNode = &node->ast[declIds[d]];
    if (declNode->type == NODE_COMMENT ||
        declNode->type == NODE_INLINE_COMMENT ||
        declNode->type == NODE_BLOCK_COMMENT) {
      /* Advance srcPos past this declaration's needle in source */
      if (needle && declStart >= 0) {
        srcPos = declStart + strlen(needle);
      }
      continue;
    }

    /* Output formatted declaration */
    fmtNode(fmt, node, declIds[d]);
    fprintf(fmt->out, "\n");
    fmt->needsIndent = false;
    fmt->lastWasNewline = true;

    /* Advance srcPos past this declaration's needle in source */
    if (needle && declStart >= 0) {
      srcPos = declStart + strlen(needle);
    }
  }

  /* Output any trailing comments */
  while (srcPos < srcLen) {
    char c = src[srcPos];
    char next = (srcPos + 1 < srcLen) ? src[srcPos + 1] : 0;

    if (c == ' ' || c == '\t' || c == '\r') {
      srcPos++;
      continue;
    }
    if (c == '\n') {
      srcPos++;
      /* Skip consecutive newlines (blank lines) */
      while (srcPos < srcLen && src[srcPos] == '\n') {
        fmtNewline(fmt);
        srcPos++;
      }
      continue;
    }
    if (c == '#' || (c == '/' && (next == '/' || next == '*'))) {
      formatComment(src, &srcPos, srcLen);
      continue;
    }
    break; /* Found code, stop */
  }

  return 0;
}

/* ==================== Public API ==================== */

int formatFile(const char *path) {
  if (!path || !*path) return 1;

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) return 1;

  if (!readfile(path, state->buffer)) {
    fprintf(stderr, "fmt: cannot read '%s'\n", path);
    gcclean();
    return 1;
  }

  Formatter fmt = {0};
  fmt.out = stdout;
  fmt.indent = 0;
  fmt.needsIndent = false;
  fmt.lastWasNewline = false;

  int result = runFormat(state, &fmt);
  gcclean();
  return result;
}

int formatString(const char *source) {
  if (!source || !*source) return 1;

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) return 1;

  Buffer *buffer = state->buffer;
  int len = strlen(source);
  if (buffer->capacity < len + 1) {
    buffer->capacity = len + 256;
    buffer->value = realloc(buffer->value, buffer->capacity);
  }
  memcpy(buffer->value, source, len);
  buffer->value[len] = '\0';
  buffer->length = len;

  Formatter fmt = {0};
  fmt.out = stdout;
  fmt.indent = 0;
  fmt.needsIndent = false;
  fmt.lastWasNewline = false;

  int result = runFormat(state, &fmt);
  gcclean();
  return result;
}

int formatStdin(void) {
  char buf[65536];
  int total = 0;
  int n;
  while ((n = fread(buf + total, 1, sizeof(buf) - total - 1, stdin)) > 0)
    total += n;
  buf[total] = '\0';
  if (total == 0) return 0;
  return formatString(buf);
}
