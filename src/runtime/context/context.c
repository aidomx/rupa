#include <rupa.h>

FlagType determineFlag(const char c) {
  if (isspace(c))
    return (c == '\n') ? FLAG_NEWLINE : FLAG_WHITESPACE;
  if (isdigit(c))
    return FLAG_NUMBER;
  if (isalpha(c) || c == '_')
    return FLAG_IDENTIFIER;
  if ((c == '"' || c == '\'') && isstr(c + 1))
    return FLAG_STRING;
  if (strchr("+-*/%=&|<>!^~", c))
    return FLAG_OPERATOR;
  if (strchr("()", c))
    return FLAG_PARENTHESIS;
  if (strchr("[]", c))
    return FLAG_BRACKET;
  if (strchr("{}", c))
    return FLAG_BRACE;
  if (c == '#')
    return FLAG_HASH;
  if (c == '@')
    return FLAG_AT;

  return FLAG_UNKNOWN;
}

FlagType detectAssignment(FlagType flag, StateInput *current) {
  // FlagType nextFlag = FLAG_UNKNOWN;

  if (flag == FLAG_IDENTIFIER) {
    printf("flag: %d\n", current->flag);
  }

  return flag;
}

StateInput *createContextInput(StateInput *input, Buffer *buffer, int line) {
  StateInput *head = NULL;
  StateInput *current = NULL;

  for (int i = 0; i < buffer->length; i++) {
    FlagType flag = determineFlag(buffer->value[i]);

    if (!head) {
      head = input;
      current = head;
    }

    head->flag = flag;
    head->line = line;
    head->row = i + 1;
    current->next = head;
    /*printf("DEBUG: (line: %d, col: %d, flag: %d)\n", current->line,*/
    /*current->row, flag);*/
  }

  return head;
}

char *getContextInput(const char *input, int start, int end) {
  if (!input || start >= end)
    return NULL;

  // malloc by gc, it's auto register
  char *buffer = gcmall(sizeof(char *));
  int length = 0;

  for (int i = start; i < end; i++) {
    if (isassign(input[i])) {
      end = i;
      continue;
    }

    else if (islparen(input[i])) {
      end = i;
      continue;
    }

    else if (islblock(input[i])) {
      end = i;
      continue;
    }

    buffer[length++] = input[i];
  }

  if (length == 0)
    return NULL;

  buffer[length] = '\0';
  switch (gettype(buffer)) {
  case RBLOCK:
    return buffer;

  case IDENTIFIER:
    return buffer;

  case NUMBER:
    return buffer;

  default:
    return NULL;
  }

  return NULL;
}

void addContextInput(State *state, char c, int line, int row) {
  if (!state)
    return;

  FlagType flag = determineFlag(c);
  printf("DEBUG: (line: %d, col: %d, char: %c, flag: %d)\n", line, row, c,
         flag);
}

int secureId(const char *input, int start, int end) {
  if (!input)
    return -1;

  int id = 1;
  for (int i = start; i < end; i++) {
    Symbol *symbol = getSymbolToken(input[i]);
    if (symbol && symbol->type != UNDERLINE) {
      id--;
      break;
    }
  }

  return id;
}

const char *skipId(const char *ptr) {
  if (!ptr)
    return NULL;

  // case: x1_
  if (isstr(*ptr)) {
    ptr++;
    while (isint(*ptr) || isunderscore(*ptr))
      ptr++;

    return skipId(ptr++);
  }

  // next case: _1x
  else if (isunderscore(*ptr)) {
    ptr++;
    while (isstr(*ptr)) {
      ptr++;

      if (isalnum(*ptr) || isunderscore(*ptr))
        ptr++;
    }
  }

  // invalid case: _1
  return ptr;
}

void setContextInput(State *state) {
  if (!gc || !state)
    return;

  state->debug->context = true;

  Input *input = state->input;
  if (input->length == 0)
    return;

  if (state->debug->context) {
  }

  /*Buffer *buf = state->repl->buffer;*/
  /*Error *error = state->error;*/
  /*Editor *ed = state->repl->editor;*/
  /*int line = ed->lineNumber;*/

  /*const char *id = skipId(buf->value);*/
  /*// prepare errorInfo*/
  /*ErrorInfo errorInfo = setErrorInfo(buf->value, NULL, line,*/
  /*buf->length - strlen(id), ERROR_NONE);*/

  /*// jika awalan bukan (*/
  /*if (!islparen(*id)) {*/
  /*// jika karakter selanjutnya identik dengan (*/
  /*if (islparen(*id + 1)) {*/
  /*errorInfo.type = ERR_INVALID_FUNCTION_NAME;*/
  /*addError(error, errorInfo);*/
  /*}*/
  /*// kembalikan indent*/
  /*ed->indentLevel = 0;*/
  /*return;*/
  /*}*/
  /*printf("%s\n", id);*/
}

void processContextInput(State *state) {
  if (!state || !state->input)
    return;

  /*Input *input = state->input;*/
  /*Context *ctx = input->context;*/
  // Keyword *k = input->keyword;
  /*for (int i = 0; i < input->length; i++) {*/
  /*printf("buffer: %d\n", input->buffer[i]);*/
  /*}*/

  /*printf("input length: %d\n", input->length);*/
  /*printf("context prev: %d\n", ctx->prev);*/

  /*switch (ctx->prev) {*/
  /*case CONTEXT_IDENTIFIER_REFERENCE:*/
  /*printf("prev: %d\n", ctx->prev);*/
  /*break;*/

  /*case CONTEXT_VAR_DECLARATION:*/
  /*printf("id: %d\n", ctx->prev);*/
  /*break;*/

  /*default:*/
  /*break;*/
  /*}*/
}
