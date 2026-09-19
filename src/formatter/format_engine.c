#include <rupa.h>

/*
 * Mesin inti formatter: memetakan declaration AST kembali ke posisi
 * source aslinya (getDeclNeedle/findDeclStart) dan menjalankan loop
 * format utama (runFormat).
 */

/* ==================== Source-based formatting ==================== */

/*
 * Extract first meaningful keyword/text from an AST node for source matching.
 */
static const char *getDeclNeedle(Node *node, int nodeId) {
  if (nodeId < 0 || nodeId >= node->length) return NULL;
  AstNode *n = &node->ast[nodeId];
  switch (n->type) {
  case NODE_COMMENT:
  case NODE_INLINE_COMMENT:
  case NODE_BLOCK_COMMENT:
    return getDeclNeedle(node, n->asComment.type);
  case NODE_IF:
    return "if";
  case NODE_LOOP:
    return n->loop.kind;
  case NODE_FUNCTION_DECL:
    return getDeclNeedle(node, n->function.name);
  case NODE_STRUCT_DECL:
    return getDeclNeedle(node, n->asStruct.name);
  case NODE_CLASS_DECL:
    return getDeclNeedle(node, n->asClass.name);
  case NODE_MARKER:
    /* @created — format sebagai statement: needle nama marker. */
    return n->asClass.name >= 0 ? getDeclNeedle(node, n->asClass.name) : "@";
  case NODE_MOD:
    return n->mod.type == ImportDecl ? "import" : "export";
  case NODE_EXTENDS:
    return "extends";
  case NODE_ASSIGN:
    return getDeclNeedle(node, n->assign.target);
  case NODE_MEMBER_ASSIGN:
    /* `p.name = ...` (juga struct/pin field write) — tanpa case ini,
     * needle jatuh ke NULL dan lookahead-comment di runFormat() gagal
     * cari titik mulai deklarasi berikutnya, membuat jendela pemindaian
     * komentar meluber sampai akhir file (semua komentar sesudahnya
     * ikut tersapu dalam satu kali proses). */
    return getDeclNeedle(node, n->memberAssign.target);
  case NODE_CONDITIONAL_ASSIGN:
    return getDeclNeedle(node, n->conditionalAssign.target);
  case NODE_ANNOTATION:
    return getDeclNeedle(node, n->annotation.name);
  case NODE_CALL:
    /* Statement-level call (assertEq(...), fn(), ...): needle = nama
     * callee. Tanpa ini needle NULL dan statement call tercetak di
     * posisi deklarasi berikutnya — urutan statement berubah. */
    return getDeclNeedle(node, n->call.callee);
  case NODE_RETURN:
    /* NODE_RETURN is also used as the wrapper for expression statements
     * (explicitReturn == false). Without this case, expression statements
     * such as `thread.sleep(100)` have no source anchor, so runFormat()
     * consumes all following source as one window and can move statements
     * across blank lines. Explicit returns use the `return` keyword to
     * avoid accidentally matching the returned identifier elsewhere. */
    if (n->asReturn.explicitReturn) return "return";
    return getDeclNeedle(node, n->asReturn.expression);
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
 * declarations that do not start a line still match. Additionally,
 * matches inside unbalanced braces are rejected: the declaration chain
 * is always top-level, so a needle that only occurs inside a
 * not-yet-consumed struct/function body (e.g. the field `name` of a
 * just-emitted struct) must not shadow the real declaration.
 */
static int braceDepthBefore(const char *src, int p) {
  int depth = 0;
  for (int i = 0; i < p; i++) {
    if (src[i] == '{')
      depth++;
    else if (src[i] == '}')
      depth--;
  }
  return depth;
}

static int findDeclStart(const char *src, int srcPos, int srcLen, const char *needle) {
  if (!needle) return -1;
  size_t nlen = strlen(needle);
  if (nlen == 0) return -1;

  int pos = srcPos;
  int fallback = -1;
  while (pos + (int)nlen <= srcLen) {
    const char *found = strstr(src + pos, needle);
    if (!found) break;
    int p = (int)(found - src);

    /* Deklarasi chain selalu top-level: tolak match di dalam body blok
     * yang belum terkonsumsi (needle "name" cocok dengan FIELD struct
     * yang baru saja dicetak — srcPos hanya melampaui needle-nya). */
    if (braceDepthBefore(src, p) != 0) {
      pos = p + 1;
      continue;
    }
    if (fallback < 0) fallback = p;

    /* Walk back over indentation; the match is at line start when the
     * preceding char is a newline (or the match begins the file). */
    int q = p;
    while (q > 0 && src[q - 1] != '\n' && (src[q - 1] == ' ' || src[q - 1] == '\t'))
      q--;
    if (q == 0 || src[q - 1] == '\n') return p;

    pos = p + 1;
  }
  return fallback;
}

int runFormat(State *state, Formatter *fmt) {
  Buffer *buffer = state->buffer;
  const char *src = buffer->value;
  int srcLen = buffer->length;

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (state->input->flags && state->input->flags->isWaiting)) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "LexerError", "no tokens produced", state->input->content,
                       state->input->cursor, ERR_UNEXPECTED_EOF);
    printErrors(state->error);
    return 1;
  }

  Request req = createRequestWithError(tokens, 10, state->error);
  Node *node = processGenerate(&req);
  if (!node || node->length <= 0) {
    if (!state->error || state->error->size == 0)
      addSourceErrorAt(state->error, "ParserError", "failed to build AST", state->input->content,
                       state->input->cursor, ERR_SYNTAX);
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
    addSourceErrorAt(state->error, "ParserError", "program root not found", state->input->content,
                     state->input->cursor, ERR_SYNTAX);
    printErrors(state->error);
    return 1;
  }

  if (fmt->config) {
    size_t normalizedLen = 0;
    char *normalized = fmtNormalizeSource(src, (size_t)srcLen, fmt->config, &normalizedLen);
    if (!normalized) return 1;
    fwrite(normalized, 1, normalizedLen, fmt->out);
    free(normalized);
    return 0;
  }

  AstNode *prog = &node->ast[root];
  AstDeclaration *current = prog->program.declarations;
#define MAX_DECLS 256
  int declIds[MAX_DECLS];
  int declCount = 0;
  {
    AstDeclaration *d = current;
    while (d && declCount < MAX_DECLS) {
      AstNode *decl = &node->ast[d->nodeId];
      /*
       * Comments are source decorations, not formatting boundaries.
       * They are emitted by the source scanner so their physical position
       * (including blank lines) is preserved exactly once. Keeping comment
       * nodes in declIds makes every comment rescan the same source window,
       * which duplicates comments/blank lines when several top-level
       * comments precede the first real declaration.
       */
      if (decl->type != NODE_COMMENT && decl->type != NODE_INLINE_COMMENT &&
          decl->type != NODE_BLOCK_COMMENT) {
        declIds[declCount++] = d->nodeId;
      }
      d = d->next;
    }
  }

  /*
   * Main formatting loop: walk source text linearly, output comments inline,
   * and output formatted AST nodes at each declaration boundary.
   *
   * pendingNewline = 1 berarti deklarasi terakhir baru saja tercetak dan
   * newline penutupnya DITUNDA — komentar inline ("x = 1 # catatan") boleh
   * menempel sebelum terminator itu ditulis. Di-flush bila window scan
   * melintasi newline/blank line, atau di akhir file.
   */
  int srcPos = 0;
  fmt->pendingNewline = 0;

  for (int d = 0; d < declCount; d++) {
    /* Find declaration start in source */
    const char *needle = getDeclNeedle(node, declIds[d]);
    int declStart = -1;
    if (needle) {
      declStart = findDeclStart(src, srcPos, srcLen, needle);
    }
    if (declStart < 0) declStart = srcLen; /* fallback: no more source */

    /* Output comments and blank lines between srcPos and declStart */
    int crossedNewline = 0;
    while (srcPos < declStart) {
      char c = src[srcPos];
      char next = (srcPos + 1 < srcLen) ? src[srcPos + 1] : 0;

      /* Skip whitespace (but track blank lines) */
      if (c == ' ' || c == '\t' || c == '\r') {
        srcPos++;
        continue;
      }

      if (c == '\n') {
        /* Newlines inside the source body of the declaration being skipped
         * are already represented by fmtNode(). Only top-level newlines are
         * separators between declarations and therefore belong to this
         * source-preservation pass. */
        if (braceDepthBefore(src, srcPos) != 0) {
          srcPos++;
          continue;
        }

        /* Check if next non-space char is also \n (blank line) */
        int peek = srcPos + 1;
        while (peek < srcLen && (src[peek] == ' ' || src[peek] == '\t'))
          peek++;
        if (peek < srcLen && src[peek] == '\n') {
          /* Blank line — preserve it only after the declaration terminator
           * has been emitted. Cap at SATU baris kosong per run (maxEmpty: 1,
           * .rupa-format): sebelumnya fmtNewline dipanggil per '\n' sehingga
           * run panjang bocor melebihi cap (dan jumlahnya tergantung posisi
           * while-loop berhenti). Scan seluruh run, whitespace di antaranya
           * termasuk ("\n \n" tetap satu run), lalu emit satu blank. */
          int scan = peek;
          while (scan < srcLen) {
            if (src[scan] == ' ' || src[scan] == '\t' || src[scan] == '\r' || src[scan] == '\n') {
              scan++;
              continue;
            }
            break;
          }
          if (fmt->pendingNewline) {
            fprintf(fmt->out, "\n");
            fmt->pendingNewline = 0;
          }
          fmtNewline(fmt);
          srcPos = scan;
        } else {
          /* Regular top-level newline: it terminates the formatted
           * declaration, so flush its deferred newline. */
          srcPos++;
          if (fmt->pendingNewline) {
            fprintf(fmt->out, "\n");
            fmt->pendingNewline = 0;
          }
        }
        crossedNewline = 1;
        continue;
      }

      /* Found a comment — format and output it. Komentar yang menempel
       * di belakang deklarasi yang baru tercetak (inline, pendingNewline
       * masih tertahan, belum melintasi newline) diberi SATU spasi lalu
       * dicetak — terminator baris datang dari formatComment sendiri. */
      if (c == '#' || (c == '/' && (next == '/' || next == '*'))) {
        if (fmt->pendingNewline && !crossedNewline) {
          fmtSep(fmt); /* spasi sebelum komentar inline */
        } else {
          if (fmt->pendingNewline) {
            fprintf(fmt->out, "\n");
            fmt->pendingNewline = 0;
          }
          fmtIndent(fmt);
        }
        formatCommentTo(fmt->out, src, &srcPos, srcLen);
        fmt->pendingNewline = 0; /* \n milik formatComment menutup baris */
        fmt->needsIndent = true;
        fmt->lastWasNewline = true;
        continue;
      }

      /* Found code — skip it (part of previous declaration) */
      srcPos++;
    }

    /* Output formatted declaration — trailing newline DITUNDA agar
     * komentar inline di window berikutnya bisa menempel. */
    /* Flush pending newline deklarasi sebelumnya bila window ini tidak
     * melampirkan komentar inline (yang sudah men-null-kan pending):
     * deklarasi dengan needle NULL (NODE_RETURN dari statement member
     * call) punya window kosong — tanpa flush di sini baris-baris
     * menyatu. */
    if (fmt->pendingNewline) {
      fprintf(fmt->out, "\n");
      fmt->pendingNewline = 0;
    }
    fmtNode(fmt, node, declIds[d]);
    fmt->needsIndent = false;
    fmt->lastWasNewline = false;
    fmt->pendingNewline = 1;

    /* Advance srcPos past this declaration's needle in source */
    if (needle && declStart >= 0) {
      srcPos = declStart + strlen(needle);
    }

    /* Komentar inline di ekor deklarasi ini (setelah needle, sebelum
     * '\n') — tempelkan sebelum terminator tertunda. Needle sering lebih
     * pendek dari deklarasi (MEMBER_ASSIGN "data" vs "data.age = 30"),
     * jadi lewati kode sisa di baris ini sampai komentar; berhenti di
     * '{' (buka blok — komentar di dalam body bukan milik sini) dan
     * guard quotes ('#'/'//' di dalam string literal bukan komentar).
     * Deklarasi terakhir tidak punya window berikutnya, jadi tanpa ini
     * komentarnya jatuh ke trailing scan sebagai baris terpisah. */
    {
      int q = srcPos;
      int commentAt = -1;
      while (q < srcLen) {
        char c = src[q];
        if (c == '\n' || c == '{') break;
        if (c == '"') {
          q++;
          while (q < srcLen && src[q] != '"' && src[q] != '\n')
            q++;
          q++;
          continue;
        }
        if (c == '#' || (c == '/' && q + 1 < srcLen && (src[q + 1] == '/' || src[q + 1] == '*'))) {
          commentAt = q;
          break;
        }
        q++;
      }
      if (commentAt >= 0 && fmt->pendingNewline) {
        fmtSep(fmt);
        int adv = commentAt;
        formatCommentTo(fmt->out, src, &adv, srcLen);
        fmt->pendingNewline = 0;
        /* srcPos ke SETELAH komentar — window deklarasi berikutnya
         * dimulai di sini, komentar tidak tercetak ulang. */
        srcPos = adv;
      }
    }
  }

  /* Flush newline deklarasi terakhir sebelum trailing scan */
  if (fmt->pendingNewline) {
    fprintf(fmt->out, "\n");
    fmt->pendingNewline = 0;
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
      /* Newline pertama menutup deklarasi terakhir; sisa run = baris
       * kosong — emit MAKSIMAL SATU (maxEmpty: 1, .rupa-format), lalu
       * lewati seluruh run (whitespace di antaranya termasuk). */
      srcPos++;
      int sawBlank = 0;
      while (srcPos < srcLen && (src[srcPos] == '\n' || src[srcPos] == ' ' || src[srcPos] == '\t' ||
                                 src[srcPos] == '\r')) {
        if (src[srcPos] == '\n') sawBlank = 1;
        srcPos++;
      }
      if (sawBlank) {
        int maxEmpty = fmt->config ? fmt->config->maxEmpty : 1;
        if (maxEmpty > 0) fmtNewline(fmt);
      }
      continue;
    }
    if (c == '#' || (c == '/' && (next == '/' || next == '*'))) {
      formatCommentTo(fmt->out, src, &srcPos, srcLen);
      continue;
    }
    break; /* Found code, stop */
  }

  return 0;
}

