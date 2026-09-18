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

  AstNode *prog = &node->ast[root];
  AstDeclaration *current = prog->program.declarations;
  if (!current) return 0; /* Collect declarations */
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
        /* Check if next non-space char is also \n (blank line) */
        int peek = srcPos + 1;
        while (peek < srcLen && (src[peek] == ' ' || src[peek] == '\t'))
          peek++;
        if (peek < srcLen && src[peek] == '\n') {
          /* Blank line — flush pending, lalu preserve blank */
          if (fmt->pendingNewline) {
            fprintf(fmt->out, "\n");
            fmt->pendingNewline = 0;
          }
          fmtNewline(fmt);
          srcPos = peek + 1;
        } else {
          /* Regular newline — flush pending newline deklarasi */
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

    /* Skip comment declarations - already output by source scanning */
    AstNode *declNode = &node->ast[declIds[d]];
    if (declNode->type == NODE_COMMENT || declNode->type == NODE_INLINE_COMMENT ||
        declNode->type == NODE_BLOCK_COMMENT) {
      /* Advance srcPos past this declaration's needle in source */
      if (needle && declStart >= 0) {
        srcPos = declStart + strlen(needle);
      }
      continue;
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
      srcPos++;
      /* Skip consecutive newlines (blank lines) */
      while (srcPos < srcLen && src[srcPos] == '\n') {
        fmtNewline(fmt);
        srcPos++;
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

/* ==================== Batch formatter / test selection ==================== */

typedef struct FmtPathList {
  char **items;
  int count;
  int capacity;
} FmtPathList;

static void fmtPathListFree(FmtPathList *list) {
  if (!list) return;
  for (int i = 0; i < list->count; i++)
    free(list->items[i]);
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

static int fmtPathListAdd(FmtPathList *list, const char *path) {
  if (!list || !path) return 1;
  if (list->count >= list->capacity) {
    int cap = list->capacity ? list->capacity * 2 : 32;
    char **items = realloc(list->items, (size_t)cap * sizeof(*items));
    if (!items) return 1;
    list->items = items;
    list->capacity = cap;
  }
  list->items[list->count] = strdup(path);
  if (!list->items[list->count]) return 1;
  list->count++;
  return 0;
}

static int fmtPathCmp(const void *a, const void *b) {
  const char *pa = *(const char *const *)a;
  const char *pb = *(const char *const *)b;
  return strcmp(pa, pb);
}

static bool fmtHasRpExtension(const char *path) {
  const char *dot = strrchr(path, '.');
  return dot && strcmp(dot, ".rp") == 0;
}

static int fmtCollectRp(const char *root, FmtPathList *list) {
  struct stat st;
  if (stat(root, &st) != 0) {
    fprintf(stderr, "fmt: path not found: %s\n", root);
    return 1;
  }
  if (S_ISREG(st.st_mode)) {
    return fmtHasRpExtension(root) ? fmtPathListAdd(list, root) : 0;
  }
  if (!S_ISDIR(st.st_mode)) return 0;

  DIR *dir = opendir(root);
  if (!dir) {
    fprintf(stderr, "fmt: cannot open directory '%s'\n", root);
    return 1;
  }

  struct dirent *entry;
  int result = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    size_t len = strlen(root) + strlen(entry->d_name) + 2;
    char *path = malloc(len);
    if (!path) {
      result = 1;
      break;
    }
    snprintf(path, len, "%s/%s", root, entry->d_name);
    result = fmtCollectRp(path, list);
    free(path);
    if (result) break;
  }
  closedir(dir);
  return result;
}

static bool fmtPathExcluded(const char *path, const char **excludes, int count) {
  for (int i = 0; i < count; i++) {
    const char *ex = excludes[i];
    if (!ex || !*ex) continue;
    size_t n = strlen(ex);
    if (strncmp(path, ex, n) == 0 &&
        (path[n] == '\0' || path[n] == '/' || (n > 0 && ex[n - 1] == '/')))
      return true;
  }
  return false;
}

static int fmtResolveTestPath(const char *path, char *out, size_t outSize) {
  if (!path || !*path || !out || outSize == 0) return 1;
  struct stat st;
  if (stat(path, &st) == 0) {
    snprintf(out, outSize, "%s", path);
    return 0;
  }
  if (strncmp(path, "tests/", 6) == 0) {
    snprintf(out, outSize, "%s", path);
    return 0;
  }
  snprintf(out, outSize, "tests/%s", path);
  return 0;
}

static void fmtPrintList(const FmtPathList *list, const char **excludes, int excludeCount) {
  int index = 0;
  for (int i = 0; i < list->count; i++) {
    if (fmtPathExcluded(list->items[i], excludes, excludeCount)) continue;
    printf("%d. %s\n", ++index, list->items[i]);
  }
  if (index == 0) printf("No .rp files found.\n");
}

/* Read a file's raw source text (malloc'd, NUL-terminated) or NULL. */
static char *fmtReadSource(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  char *text = NULL;
  if (fseek(fp, 0, SEEK_END) == 0) {
    long size = ftell(fp);
    if (size >= 0) {
      rewind(fp);
      text = malloc((size_t)size + 1);
      if (text) {
        size_t got = fread(text, 1, (size_t)size, fp);
        text[got] = '\0';
      }
    }
  }
  fclose(fp);
  return text;
}

/* Run the formatter over one file into `out`. Owns its GC cleanup;
 * caller must reinitialize GC before each invocation in batch mode. */
static int fmtRunFile(const char *path, FILE *out) {
  if (!path || !*path || !out) return 1;

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) return 1;

  if (!readfile(path, state->buffer)) {
    fprintf(stderr, "fmt: cannot read '%s'\n", path);
    gcclean();
    return 1;
  }

  Formatter fmt = {0};
  fmt.out = out;
  fmt.indent = 0;
  fmt.needsIndent = false;
  fmt.lastWasNewline = false;

  int result = runFormat(state, &fmt);
  gcclean();
  return result;
}

/* Format one file into a malloc'd buffer (caller frees with free()). */
static int fmtFormatToBuffer(const char *path, char **out, size_t *outLen) {
  *out = NULL;
  if (outLen) *outLen = 0;

  char *buf = NULL;
  size_t len = 0;
  FILE *fp = open_memstream(&buf, &len);
  if (!fp) return 1;

  int result = fmtRunFile(path, fp);
  if (fclose(fp) != 0 && result == 0) result = 1;
  if (result != 0) {
    free(buf);
    return result;
  }
  *out = buf;
  if (outLen) *outLen = len;
  return 0;
}

static int fmtRunPaths(const FmtPathList *list, const int *selected, int selectedCount,
                       const char **excludes, int excludeCount) {
  int result = 0;
  int listed = 0;
  int printed = 0;
  int modified = 0;
  int unchanged = 0;
  int failed = 0;
  for (int i = 0; i < list->count; i++) {
    if (fmtPathExcluded(list->items[i], excludes, excludeCount)) continue;
    /* Numbering counts non-excluded entries so indexes match fmtPrintList(). */
    listed++;
    bool use = selectedCount == 0;
    if (!use) {
      for (int j = 0; j < selectedCount; j++) {
        if (selected[j] == listed) {
          use = true;
          break;
        }
      }
    }
    if (!use) continue;

    if (printed++) printf("\n");

    char *source = fmtReadSource(list->items[i]);
    char *formatted = NULL;
    size_t formattedLen = 0;
    /* fmtRunFile owns its GC cleanup; reinitialize it before each file
     * when batch mode invokes it repeatedly. */
    gcinit(100);
    int r = source ? fmtFormatToBuffer(list->items[i], &formatted, &formattedLen) : 1;
    if (r != 0) {
      failed++;
      result = r;
      printf("%-9s %s\n", "FAILED", list->items[i]);
      free(source);
      free(formatted);
      continue;
    }

    bool changed = !source || formattedLen != strlen(source) ||
                   memcmp(source, formatted, formattedLen) != 0;
    printf("%-9s %s\n", changed ? "MODIFIED" : "UNCHANGED", list->items[i]);
    if (changed)
      modified++;
    else
      unchanged++;
    fwrite(formatted, 1, formattedLen, stdout);
    if (formattedLen == 0 || formatted[formattedLen - 1] != '\n') printf("\n");
    free(source);
    free(formatted);
  }
  if (!printed) {
    fprintf(stderr, "fmt: no selected .rp files\n");
    return 1;
  }
  printf("\n%d file(s): %d modified, %d unchanged", printed, modified, unchanged);
  if (failed) printf(", %d failed", failed);
  printf("\n");
  return result;
}

int formatList(const char *path, bool listOnly) {
  char resolved[4096];
  if (fmtResolveTestPath(path ? path : "tests", resolved, sizeof(resolved)) != 0) return 1;
  FmtPathList list = {0};
  int result = fmtCollectRp(resolved, &list);
  if (result == 0) qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
  if (result == 0) {
    if (listOnly)
      fmtPrintList(&list, NULL, 0);
    else
      result = fmtRunPaths(&list, NULL, 0, NULL, 0);
  }
  fmtPathListFree(&list);
  return result;
}

static int fmtParseCsvInts(const char *value, int **out, int *count) {
  *out = NULL;
  *count = 0;
  if (!value || !*value) return 1;
  char *copy = strdup(value);
  if (!copy) return 1;
  int capacity = 8;
  int *items = malloc((size_t)capacity * sizeof(*items));
  if (!items) {
    free(copy);
    return 1;
  }
  char *save = NULL;
  for (char *tok = strtok_r(copy, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    char *end = NULL;
    long n = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || n <= 0 || n > INT_MAX) {
      free(items);
      free(copy);
      return 1;
    }
    if (*count >= capacity) {
      capacity *= 2;
      int *tmp = realloc(items, (size_t)capacity * sizeof(*items));
      if (!tmp) {
        free(items);
        free(copy);
        return 1;
      }
      items = tmp;
    }
    items[(*count)++] = (int)n;
  }
  free(copy);
  *out = items;
  return *count > 0 ? 0 : 1;
}

int formatSelect(const char *select, const char *path, const char **excludes, int excludeCount) {
  FmtPathList list = {0};
  int *selected = NULL;
  int selectedCount = 0;
  char root[4096];
  int result = fmtResolveTestPath(path && *path ? path : "tests", root, sizeof(root));
  if (result == 0) result = fmtCollectRp(root, &list);
  if (result == 0) result = fmtParseCsvInts(select, &selected, &selectedCount);
  if (result == 0) {
    qsort(list.items, (size_t)list.count, sizeof(*list.items), fmtPathCmp);
    /* Valid range follows the post-exclusion numbering shown by --list. */
    int available = 0;
    for (int i = 0; i < list.count; i++) {
      if (!fmtPathExcluded(list.items[i], excludes, excludeCount)) available++;
    }
    for (int i = 0; i < selectedCount; i++) {
      if (selected[i] > available) {
        fprintf(stderr, "fmt: selection %d is out of range (1-%d)\n", selected[i], available);
        result = 1;
        break;
      }
    }
  }
  if (result == 0) result = fmtRunPaths(&list, selected, selectedCount, excludes, excludeCount);
  free(selected);
  fmtPathListFree(&list);
  return result;
}

/* ==================== Public API ==================== */

int formatFile(const char *path) {
  /* Single-file mode keeps the raw formatter output (no batch status). */
  return fmtRunFile(path, stdout);
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
