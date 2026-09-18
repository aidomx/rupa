#include <rupa.h>

static TokenType last_token_type(State *state) {
  if (!state || !state->tokens || state->tokens->length == 0) return UNKNOWN;
  return state->tokens->data[state->tokens->length - 1].type;
}

static bool parseComment(Token *t, const char *s, int *p, int end, int line) {
  char c = s[*p];
  char next = (*p + 1 < end) ? s[*p + 1] : 0;

  /* Single-line comment : # or // */
  if (c == '#' || (c == '/' && next == '/')) {
    int start = *p;
    int marker = (c == '#') ? 1 : 2; /* skip # or // */
    int pos = start + marker;
    while (pos < end && s[pos] != '\n')
      pos++;
    *p = pos;

    addDelim(t, c, NULL, line, start);
    char *str = substring(s, start + marker, pos);
    addToken(t, createDataToken(str, NULL, COMMENT, line, pos));

    if (*p < end && s[*p] == '\n') return true;
    return true;
  }

  /* Block comment: slash-star ... star-slash */
  if (c == '/' && next == '*') {
    int start = *p;
    /* Advance past opening block comment marker */
    (*p) += 2;

    /* Scan for closing block comment end marker */
    while (*p < end - 1) {
      if (s[*p] == '*' && s[*p + 1] == '/') {
        (*p) += 2; /* advance past end marker */
        addDelim(t, '/', NULL, line, start);
        char *str = substring(s, start, *p);
        addToken(t, createDataToken(str, NULL, COMMENT, line, *p));
        return true;
      }
      (*p)++;
    }
    /* Unterminated block comment — consume to end of input */
    addDelim(t, '/', NULL, line, start);
    char *str = substring(s, start, end);
    addToken(t, createDataToken(str, NULL, COMMENT, line, end));
    *p = end;
    return true;
  }

  return false;
}

int processConstruct(State *state, int start, int end, bool *waiting) {
  if (!state || !state->input || !state->tokens || !waiting) return -1;

  const char *s = state->input->content;
  int p = start;
  StateContext *ctx = state->context;
  int brace = ctx ? ctx->brace : 0;
  int bracket = ctx ? ctx->bracket : 0;
  int paren = ctx ? ctx->paren : 0;
  bool expectValue = false;
  bool singleStatement = ctx && ctx->colon > 0;
  bool statementStarted = false;

  while (p < end) {
    char c = s[p];
    /* Keep token locations tied to the actual source cursor. The old
     * input->line value is REPL-oriented and remains 0 for file input, which
     * made every lexer/parser/runtime error lose its source line. */
    int line = 1, row = 1;
    for (int i = 0; i < p; i++) {
      if (s[i] == '\n') {
        line++;
        row = 1;
      } else {
        row++;
      }
    }
    state->input->line = line;
    state->input->row = row;

    if (c == ' ' || c == '\t' || c == '\r') {
      p++;
      continue;
    }

    /* Comments are tokenized by the lexer. After parseComment returns,
       *p points to the next character. Let the main loop handle it
       naturally so NEWLINE tokens are emitted as statement separators. */
    if (parseComment(state->tokens, s, &p, end, state->input->line)) {
      continue;
    }

    if (c == '\n') {
      /* `:` selalu membatasi body satu statement fisik, termasuk ketika
         statement berada di dalam `{ ... }`. Tangani lebih dulu sebelum
         newline biasa diabaikan karena brace depth. */
      if (singleStatement) {
        /* Keep physical statement boundaries in the token stream.  The smart
         * lexer may consider `:` bodies complete, but the parser still needs
         * NEWLINE to prevent the following statement being absorbed. */
        addDelim(state->tokens, '\n', NULL, state->input->line, p++);
        singleStatement = false;
        if (ctx) ctx->colon = 0;
        continue;
      }
      /* NEWLINE tetap penting sebagai batas statement di dalam `{ ... }`.
       * Array dan argument masih boleh multiline, sehingga hanya `[]` dan
       * `()` yang menyerap newline sebagai whitespace internal. Untuk object,
       * NEWLINE aman dipertahankan karena parser grammar sudah
       * memperlakukannya sebagai whitespace pada expression. */
      /* Newline inside arrays, argument lists, and object literals is
       * structural whitespace. A normal `{ ... }` block still keeps NEWLINE
       * as a statement boundary, while an object literal is identified by
       * objectDepth. This is especially important after a property comma:
       *
       *   people: People = {
       *     name: "rupa",
       *     age: 20
       *   }
       *
       * The comma expects the next value, but the physical newline must not
       * make the smart lexer think the expression is incomplete. */
      if (bracket || paren || (ctx && ctx->objectDepth > 0 && brace >= ctx->objectDepth)) {
        p++;
        continue;
      }
      if (expectValue) {
        /* Bare `return` diikuti newline: di file mode (non-REPL) tanpa
         * delimiter terbuka, newline mengakhiri statement — return
         * telanjang sah (mengembalikan null), dipakai untuk early-exit
         * (mis. `foo(): void { return }`). REPL tetap menunggu karena
         * kelanjutan multiline (`return {` dst.) disatukan di sana.
         * Cek token terakhir, bukan flag: `return 1` (last = NUMBER)
         * tetap jalur waiting lama. */
        Token *tk = state->tokens;
        bool bareReturn = tk && tk->length > 0 &&
                          tk->data[tk->length - 1].type == KEYWORD &&
                          !strcmp(tk->data[tk->length - 1].value, "return");
        /* brace > 0 sah: bare return memang hidup di dalam block body
         * fungsi, dan newline di block = boundary statement. Yang tidak
         * boleh: di dalam [] / () (newline hanya whitespace di sana). */
        if (bareReturn && !state->isRepl && !bracket && !paren) {
          addDelim(state->tokens, '\n', NULL, state->input->line, p++);
          expectValue = false;
          *waiting = false;
          continue;
        }
        *waiting = true;
        if (ctx) {
          ctx->brace = brace;
          ctx->bracket = bracket;
          ctx->paren = paren;
        }
        return p;
      }
      addDelim(state->tokens, '\n', NULL, state->input->line, p++);
      continue;
    }

    KeywordType keywordType = KEYWORD_NONE;
    int keywordNext = p;
    if (scanKeyword(s, p, end, &keywordType, &keywordNext)) {
      int next = processKeyword(state, keywordType, p, keywordNext, end, waiting);
      if (next < 0) return -1;
      p = next;
      /* Keywords that are followed by a value expression (return, async)
       * must leave expectValue true so that a subsequent '{' is recognised
       * as an object literal (objectDepth) rather than a block. */
      expectValue = (keywordType == KEYWORD_RETURN || keywordType == KEYWORD_ASYNC);
      if (singleStatement) statementStarted = true;
      continue;
    }

    if (isquote(c)) {
      int next = p;
      if (processString(state, p, end, &next, waiting) < 0) {
        if (*waiting) {
          if (ctx) {
            ctx->brace = brace;
            ctx->bracket = bracket;
            ctx->paren = paren;
          }
          return p;
        }
        return -1;
      }
      p = next;
      expectValue = false;
      if (singleStatement) statementStarted = true;
      continue;
    }

    if (isdigit((unsigned char)c)) {
      int next = p;
      if (processNumber(state, p, end, &next, waiting) < 0) {
        if (*waiting) {
          if (ctx) {
            ctx->brace = brace;
            ctx->bracket = bracket;
            ctx->paren = paren;
          }
          return p;
        }
        return -1;
      }
      p = next;
      expectValue = false;
      if (singleStatement) statementStarted = true;
      continue;
    }

    if (isalpha((unsigned char)c) || c == '_') {
      int next = p;
      if (processIdentifier(state, p, end, expectValue, &next) < 0) return -1;
      p = next;
      expectValue = false;
      if (singleStatement) statementStarted = true;
      continue;
    }

    int next = p;
    if (strchr("()[]{}:,;", c)) {
      /* processDelimiter() mutates expectValue as a side effect (e.g. for
       * '{' it always resets it to false), so the pre-delimiter value must
       * be captured first. It tells us whether this '{' opens in a "value
       * position" (after '=', '(', '[', ',', ':') — which is exactly when a
       * brace is an object literal rather than a struct/function/block
       * body. Relying only on "previous token == ASSIGN" (as before) missed
       * every other value position: array elements, call arguments, and
       * nested object values, causing the lexer to misread their ':' as a
       * type-annotation/single-statement colon instead of a property
       * separator, and fail outright on non-word values like strings. */
      bool wasExpectingValue = expectValue;
      if (processDelimiter(state, p, end, &next, &brace, &bracket, &paren, &expectValue) < 0)
        return -1;
      if (ctx) {
        ctx->brace = brace;
        ctx->bracket = bracket;
        ctx->paren = paren;
        /* Reset marker return-type setelah '{' body dimakan — marker
         * hanya hidup untuk `): Type {` (grammar_function membaca). */
        if (c == '{') state->input->flags->isReturnType = false;
        if (c == '{') {
          TokenType previous = state->tokens->length > 1
                                   ? state->tokens->data[state->tokens->length - 2].type
                                   : UNKNOWN;
          ctx->inStruct =
              (previous == IDENTIFIER && paren == 0 && !state->input->flags->isAssignment)
                  ? 1
                  : ctx->inStruct;
          ctx->objectDepth = (previous == ASSIGN || wasExpectingValue) ? brace : ctx->objectDepth;
          if (ctx->inStruct) state->input->flags->isStructDecl = true;
        } else if (c == ':') {
          /* A colon inside an object is a property separator, not a
             single-statement marker. */
          if (ctx->objectDepth <= 0 || brace < ctx->objectDepth) {
            /* Return-type annotation (design/rupa_types_const_void_bigint.txt
             * poin 3): `foo(): void { }` / `foo(): number { }` — colon
             * langsung setelah ')' tutup parameter BUKAN colon-body
             * satu statement. Tandai & lewati: token ':' tetap diemit
             * (processDelimiter di atas), tapi jangan aktifkan
             * singleStatement agar `void { }` tidak dibaca sebagai body. */
            TokenType prev = state->tokens->length > 0
                                 ? state->tokens->data[state->tokens->length - 1].type
                                 : UNKNOWN;
            if (prev == RPAREN && paren == 0) {
              state->input->flags->isReturnType = true;
            } else {
              ctx->colon = 1;
              singleStatement = true;
            }
          }
        } else if (c == '}') {
          if (ctx->objectDepth > 0 && brace < ctx->objectDepth)
            ctx->objectDepth = brace > 0 ? brace : 0;
          if (ctx->inStruct && brace == 0) ctx->inStruct = 0;
          if (brace == 0) {
            ctx->colon = 0;
            singleStatement = false;
          }
        }
      }
      if (singleStatement && c != ':') statementStarted = true;
      p = next;
      continue;
    }

    if (strchr("=?!+-*/%|&<>^~.", c)) {
      bool opWaiting = false;
      if (processOperator(state, p, end, &next, &opWaiting) < 0) {
        *waiting = opWaiting;
        if (opWaiting) return next;
        return -1;
      }
      p = next;
      /* Most operators start or continue an expression and therefore expect
       * a value. Postfix update operators are complete statements by
       * themselves: `i++` / `i--` must be allowed to end at NEWLINE. */
      TokenType op = last_token_type(state);
      expectValue = op != INCREMENT && op != DECREMENT;
      if (singleStatement) statementStarted = true;
      continue;
    }

    return -1;
  }

  if (ctx) {
    ctx->brace = brace;
    ctx->bracket = bracket;
    ctx->paren = paren;
    ctx->colon = singleStatement ? 1 : 0;
  }

  if (singleStatement) {
    if (!statementStarted || expectValue || brace || bracket || paren) {
      *waiting = true;
      return p;
    }
    if (ctx) ctx->colon = 0;
    *waiting = false;
    return p;
  }

  if (brace || bracket || paren || expectValue) {
    /* EOF dengan expectValue: bare `return` tanpa newline di akhir file
     * tetap statement sah (file mode). Kondisi lain (assignment/operator
     * yang masih menunggu value, delimiter terbuka) tetap waiting. */
    Token *tk = state->tokens;
    bool bareReturn = expectValue && !bracket && !paren &&
                      !state->isRepl && tk && tk->length > 0 &&
                      tk->data[tk->length - 1].type == KEYWORD &&
                      !strcmp(tk->data[tk->length - 1].value, "return");
    if (!bareReturn) {
      *waiting = true;
      return p;
    }
  }

  *waiting = false;
  return p;
}
