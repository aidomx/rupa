#include <rupa.h>

int processDelimiter(State *state, int start, int end, int *next, int *brace,
                     int *bracket, int *paren, bool *expectValue) {
  (void)end;
  char c = state->input->content[start];

  switch (c) {
  case '(': {
    (*paren)++;

    /* Parentheses after `print` are an argument list, not a function call.
       The arguments themselves still use the normal construct path, so nested
       calls, expressions and commas are handled by the same delimiter state. */
    bool printArguments = state->tokens->length > 0 &&
                          state->input->flags &&
                          state->input->flags->isPrint &&
                          state->tokens->data[state->tokens->length - 1].type == KEYWORD;

    if (!printArguments && state->tokens->length > 0)
      state->input->flags->isFunctionCall = true;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = true;
    break;
  }
  case ')':
    if (!*paren)
      return -1;
    (*paren)--;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = false;
    break;
  case '[':
    (*bracket)++;
    state->input->flags->isArray = true;
    if (state->tokens->length > 1 &&
        state->tokens->data[state->tokens->length - 1].type == IDENTIFIER)
      state->input->flags->isSubs = true;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = true;
    break;
  case ']':
    if (!*bracket)
      return -1;
    (*bracket)--;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = false;
    break;
  case '{':
    (*brace)++;
    state->input->flags->isBlockProgram = true;
    if (state->tokens->length > 0 &&
        state->tokens->data[state->tokens->length - 1].type == RPAREN)
      state->input->flags->isFunctionDecl = true;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = false;
    break;
  case '}':
    if (!*brace)
      return -1;
    (*brace)--;
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = false;
    break;
  case ':':
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = true;
    break;
  case ',':
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = true;
    break;
  case ';':
    addDelim(state->tokens, c, NULL, state->input->line, start);
    *expectValue = false;
    break;
  default:
    return -1;
  }

  *next = start + 1;
  return 0;
}
