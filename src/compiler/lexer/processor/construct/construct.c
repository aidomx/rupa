#include <rupa.h>

static TokenType last_token_type(State *state) {
  if (!state || !state->tokens || state->tokens->length == 0)
    return UNKNOWN;
  return state->tokens->data[state->tokens->length - 1].type;
}

int processConstruct(State *state, int start, int end, bool *waiting) {
  if (!state || !state->input || !state->tokens || !waiting)
    return -1;

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
    if (c == ' ' || c == '\t' || c == '\r') {
      p++;
      continue;
    }

    /* Line comments are ignored by the lexer.  They must not invalidate
       tokens that were already accepted before '#'. */
    if (c == '#') {
      while (p < end && s[p] != '\n')
        p++;
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
      if (bracket || paren ||
          (ctx && ctx->objectDepth > 0 && brace >= ctx->objectDepth)) {
        p++;
        continue;
      }
      if (expectValue) {
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
      if (next < 0)
        return -1;
      p = next;
      expectValue = false;
      if (singleStatement)
        statementStarted = true;
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
      if (singleStatement)
        statementStarted = true;
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
      if (singleStatement)
        statementStarted = true;
      continue;
    }

    if (isalpha((unsigned char)c) || c == '_') {
      int next = p;
      if (processIdentifier(state, p, end, expectValue, &next) < 0)
        return -1;
      p = next;
      expectValue = false;
      if (singleStatement)
        statementStarted = true;
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
      if (processDelimiter(state, p, end, &next, &brace, &bracket, &paren,
                           &expectValue) < 0)
        return -1;
      if (ctx) {
        ctx->brace = brace;
        ctx->bracket = bracket;
        ctx->paren = paren;
        if (c == '{') {
          TokenType previous =
              state->tokens->length > 1
                  ? state->tokens->data[state->tokens->length - 2].type
                  : UNKNOWN;
          ctx->inStruct = (previous == IDENTIFIER && paren == 0 &&
                           !state->input->flags->isAssignment)
                              ? 1
                              : ctx->inStruct;
          ctx->objectDepth = (previous == ASSIGN || wasExpectingValue)
                                  ? brace
                                  : ctx->objectDepth;
          if (ctx->inStruct)
            state->input->flags->isStructDecl = true;
        } else if (c == ':') {
          /* A colon inside an object is a property separator, not a
             single-statement marker. */
          if (ctx->objectDepth <= 0 || brace < ctx->objectDepth) {
            ctx->colon = 1;
            singleStatement = true;
          }
        } else if (c == '}') {
          if (ctx->objectDepth > 0 && brace < ctx->objectDepth)
            ctx->objectDepth = 0;
          if (ctx->inStruct && brace == 0)
            ctx->inStruct = 0;
          if (brace == 0) {
            ctx->colon = 0;
            singleStatement = false;
          }
        }
      }
      if (singleStatement && c != ':')
        statementStarted = true;
      p = next;
      continue;
    }

    if (strchr("=?!+-*/%|&<>^~.", c)) {
      bool opWaiting = false;
      if (processOperator(state, p, end, &next, &opWaiting) < 0) {
        *waiting = opWaiting;
        if (opWaiting)
          return next;
        return -1;
      }
      p = next;
      /* Most operators start or continue an expression and therefore expect
       * a value. Postfix update operators are complete statements by
       * themselves: `i++` / `i--` must be allowed to end at NEWLINE. */
      TokenType op = last_token_type(state);
      expectValue = op != INCREMENT && op != DECREMENT;
      if (singleStatement)
        statementStarted = true;
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
    if (ctx)
      ctx->colon = 0;
    *waiting = false;
    return p;
  }

  if (brace || bracket || paren || expectValue) {
    *waiting = true;
    return p;
  }

  *waiting = false;
  return p;
}
