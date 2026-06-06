#include <rupa.h>

int setDeepSubscript(State *s, int start, int end) {
  if (!s)
    return -1;

  Buffer *buf = s->repl->buffer;
  Editor *ed = s->repl->editor;

  char *input = buf->value;
  char inner[1024];
  int length = 0;

  for (int i = start; i < end; i++) {
    char c = input[i];

    if (issymvalue(c)) {
      if (length > 0) {
        inner[length] = '\0';
        addToken(s->tokens, createDataToken(inner, NULL, gettype(inner),
                                            ed->lineNumber, i));
        length = 0;
      }

      addDelim(s->tokens, c, NULL, ed->lineNumber, i);
      continue;
    }
    inner[length++] = input[i];
  }

  inner[length] = '\0';
  TokenType type = gettype(inner);
  type = type == IDENTIFIER ? LITERAL_ID : type;

  return addToken(s->tokens,
                  createDataToken(inner, NULL, type, ed->lineNumber, length));
}

int setSubscript(State *state, int start, int end) {
  if (!state || start >= end)
    return -1;

  return setDeepSubscript(state, start, end);
}

int setDeepTokenId(State *s, int start, int end) {
  if (!s || start >= end)
    return -1;

  Buffer *buf = s->repl->buffer;
  Editor *ed = s->repl->editor;

  int capacity = 10;
  Position *subscripts = malloc(capacity * sizeof(Position));

  char *input = buf->value;
  int size = 0, i = start;

  while (i < end) {
    if (isassign(input[i])) {
      if (isrbracket(input[i - 1])) {
        end = i++;
        break;
      }

      end = i++;
      break;
    }

    if (islbracket(input[i]) || islblock(input[i])) {
      Position pos = getSymbolIndex(input, i, end);

      if (pos.start > 0 && pos.end > 0) {
        if (size >= capacity) {
          int newCapacity = capacity * 2;

          Position *newSubscripts =
              realloc(subscripts, newCapacity * sizeof(Position));
          if (!newSubscripts) {
            free(newSubscripts);
            return -1;
          }

          subscripts = newSubscripts;
        }

        subscripts[size++] = pos;
        i = pos.end;
      }
    }

    i++;
  }

  if (size == 0) {
    int tokenId = createTokenId(s, start, end);
    if (tokenId == -1)
      return -1;

    addDelim(s->tokens, input[end], NULL, ed->lineNumber, end);
    free(subscripts);
    return end;
  }

  int tokenId = createTokenId(s, start, subscripts[0].start);
  if (tokenId == -1) {
    free(subscripts);
    return -1;
  }

  for (int index = 0; index < size; index++) {
    Position pos = subscripts[index];

    char prev = input[pos.start], next = input[pos.end];

    addDelim(s->tokens, prev, NULL, ed->lineNumber, pos.start);
    setSubscript(s, pos.start + 1, pos.end);
    addDelim(s->tokens, next, NULL, ed->lineNumber, pos.end);
  }

  addDelim(s->tokens, input[end], NULL, ed->lineNumber, end);

  free(subscripts);
  return end;
}

int handleTokenId(State *state, int start, int end) {
  if (!state)
    return -1;

  Buffer *buf = state->repl->buffer;
  // Editor *ed = state->repl->editor;

  char *input = buf->value;
  char ptr = input[start];

  if (!(isstr(ptr) || isunderscore(ptr)))
    return -1;

  return setDeepTokenId(state, start, end);
}

int handleTokenValue(State *state, int start, int end) {
  if (!state)
    return -1;

  Buffer *buf = state->repl->buffer;
  Editor *ed = state->repl->editor;

  Token *tokens = state->tokens;
  int line = ed->lineNumber;
  char *value = getTokenValue(buf->value, start, end);

  if (!value)
    return start;

  char *ptr = value;
  char input[1024];
  int length = 0;

  for (int i = 0; ptr[i]; i++) {
    if (issymvalue(ptr[i])) {
      if (length > 0) {
        input[length] = '\0';
        addToken(tokens, createDataToken(input, NULL, gettype(input), line, i));
        length = 0;
      }

      addDelim(tokens, ptr[i], NULL, line, i);
      continue;
    }
    input[length++] = ptr[i];
  }

  input[length] = '\0';
  TokenType type = gettype(input);
  type = type == IDENTIFIER ? LITERAL_ID : type;

  int result =
      addToken(tokens, createDataToken(input, NULL, type, line, length));

  free(value);
  return result;
}

int handleNextToken(State *state, int start, int end) {
  if (!state)
    return -1;

  start = handleTokenId(state, start, end);

  if (start == -1)
    return -1;

  return handleTokenValue(state, start + 1, end);
}

// Call from src/tokenize/main.c
int processToken(State *state) {
  if (!state)
    return -1;

  Buffer *buf = state->repl->buffer;
  Editor *ed = state->repl->editor;

  char *input = buf->value;
  int hasAssign = 0, hasBlock = 0;
  int start = 0;
  int end = ed->cursorPos;

  for (int i = 0; i < end; i++) {
    if (isnewline(input[i])) {
      i++;
      start = i;
      continue;
    }

    if (isassign(input[i])) {
      if (!hasAssign) {
        hasAssign = 1;
      }
    }
  }

  // Jika token adalah atom
  if (!hasAssign && !hasBlock) {
    char token[64];
    int length = 0;
    for (int i = start; i < end; i++) {
      token[length++] = input[i];
    }

    if (length > 0) {
      token[length] = '\0';

      return addToken(
          state->tokens,
          createDataToken(token, NULL, gettype(token), ed->lineNumber, start));
    }

    return -1;
  }

  return handleNextToken(state, start, end);
}

static int handleSubs(Atom *atom, Flags *flags, LexerState *lex);

bool is_atom(Atom *atom, Flags *flags) {
  return !atom->has_next && check_terminate(atom->next) && !flags->isWaiting;
}

int handleValue(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  printf("char: %c\n", atom->next);

  return lex->cursor;
}

int handleAssign(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;
  lex->end++;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  if (check_terminate(atom->next)) {
    save_delim('\n', lex);
    return await(EXCEPT_ASSIGNMENT, flags, lex->end, true);
  }

  if (!isparen(atom->next) && !isblock(atom->next) && !isbracket(atom->next) &&
      !isoperator(atom->next)) {
    skipWhitespace(content, &lex->end);
    return handleValue(atom, flags, lex);
  }

  flags->isSubs = true;
  return handleSubs(atom, flags, lex);
}

int handleDeepSubs(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;
  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  if (flags->isSubs) {
    switch (atom->next) {
    case '[':
      atom->depth_bracket++;
      save_delim(atom->next, lex);
      break;

    case ']':
      atom->depth_bracket--;
      save_delim(atom->next, lex);
      flags->isSubs = (atom->depth_bracket == 0) && false;
      break;

    case '(':
      atom->depth_paren++;
      save_delim(atom->next, lex);
      break;

    case ')':
      atom->depth_paren--;
      save_delim(atom->next, lex);
      flags->isSubs = (atom->depth_paren == 0) && false;
      break;

    default:
      flags->except = EXCEPT_PROGRAM;
      save_rhs(atom, flags, lex);
      break;
    }
  }

  if (atom->next == '=') {
    flags->isAssignment = true;
    // save_delim(atom->next, lex);
    return handleAssign(atom, flags, lex);
  }

  if (check_terminate(atom->next)) {
    save_delim('\n', lex);
    return lex->cursor;
  }

  lex->end++;
  return handleDeepSubs(atom, flags, lex);
}

int handleSubs(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;
  atom->prev = atom->next;
  skipWhitespace(content, &lex->end);
  atom->next = content[lex->end];

  if (atom->prev == '[' && check_terminate(atom->next))
    return await(EXCEPT_SUBS, flags, lex->end, true);

  atom->depth_bracket++;
  return handleDeepSubs(atom, flags, lex);
}

int handleExcept(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  printf("except\n");

  return lex->cursor;
}

int handleProgramValue(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  const char *content = lex->content;
  skipWhitespace(content, &lex->end);

  if (flags->isAssignment)
    return handleAssign(atom, flags, lex);

  else if (flags->isSubs)
    return handleSubs(atom, flags, lex);

  return lex->cursor;
}

int nextProgram(Atom *atom, Flags *flags, LexerState *lex) {
  if (!atom || !flags || check_lexer(lex))
    return -1;

  if (is_atom(atom, flags)) {
    lex->end--;
    atom->next = lex->content[lex->end];
    flags->isComplete = saveToken(lex) != -1;
    save_delim(atom->next, lex);
    return lex->cursor;
  }

  if (flags->except != EXCEPT_NONE && flags->isWaiting)
    return handleExcept(atom, flags, lex);

  int start = save_lhs(atom, lex);
  // reset temporary
  if (atom->number || atom->string || atom->underscore || start != -1) {
    atom->number = false;
    atom->string = false;
    atom->underscore = false;
  }

  /*debug_char(atom->prev, atom->next);*/
  /*debug_pos("program", start, lex->end);*/

  switch (atom->next) {
  case '=':
    flags->isAssignment = true;
    lex->end++;
    return handleAssign(atom, flags, lex);

  case '[':
    flags->isSubs = true;
    break;
  }

  lex->end++;
  return handleProgramValue(atom, flags, lex);
}

void *handleProgram(State *state) {
  if (check_state(state))
    return NULL;

  Editor *editor = state->repl->editor;
  Input *input = state->input;
  Flags *flags = input->flags;
  Token *token = state->tokens;

  Atom atom = init_atom(input);
  LexerState lex = init_lex(editor, input, token);

  const char *content = input->content;

  atom.prev = atom.next;
  lex.start = atom.cursor;
  primaryAtom(&atom, content, input->flags);
  lex.end = atom.cursor;
  int pos = atom.cursor;
  lex.cursor = consumeToEnd(content, pos);

  // is not identifier
  if (!flags->isIdentifier && !flags->isWaiting) {
    atom.next = content[pos];
    flags->isComplete = saveToken(&lex) != -1;
    // for symbol only
    // ex: ]]
    input->cursor = resolveSymbol(&atom, flags, &lex);
    return state;
  }

  input->cursor = nextProgram(&atom, flags, &lex);
  flags->isComplete = !flags->isWaiting;
  return state;
}

void *process(State *state, const char *args) {
  if (check_state(state))
    return NULL;

  if (strcmp(args, "program") == 0)
    return handleProgram(state);

  return state;
}
