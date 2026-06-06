#include <rupa.h>

bool check_terminate(char c) { return isnewline(c) || c == '\0'; }

int await(ExceptType except, Flags *flags, int end_pos, bool prepend) {
  if (!flags)
    return -1;

  flags->except = except;
  flags->isWaiting = prepend;
  return end_pos;
}

int relex(Atom *atom, ExceptType except, LexerState *lex) {
  if (!atom || check_lexer(lex))
    return -1;

  const char *content = lex->content;

  atom->cursor = lex->end;
  lex->start = lex->end;
  int result = lexeme(atom, content, except);
  lex->end = atom->cursor;

  return result;
}

void *resolve(State *state, const char *args) {
  if (check_state(state))
    return NULL;

  if (strcmp(args, "keyword") == 0)
    return resolveKeyword(state);
  else if (strcmp(args, "program") == 0)
    return resolveProgram(state);

  return NULL;
}
