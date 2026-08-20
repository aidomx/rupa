#include <rupa.h>

Input *createInput(int capacity) {
  Input *input = gcmall(capacity * sizeof(Input));
  input->capacity = capacity;
  /*input->buffer = gcmall(MAX_BUFFER_SIZE * sizeof(char));*/
  input->content = gcmall(MAX_BUFFER_SIZE * sizeof(char));
  /*if (!input->buffer) {*/
  /*free(input->buffer);*/
  /*return NULL;*/
  /*}*/

  if (!input->content) {
    gcfree(input->content);
    return NULL;
  }
  /*input->context = createContext(capacity * sizeof(Context));*/
  input->flags = createFlags(capacity * sizeof(Flags));
  input->keyword = createKeyword();
  /*input->validation = createValidationInput(capacity *
   * sizeof(ValidationInput));*/
  input->cursor = 0;
  input->length = 0;
  input->line = 0;
  input->row = input->cursor;
  input->next = NULL;
  return input;
}

StateInput *createStateInput() {
  StateInput *input = gcmall(sizeof(StateInput));
  input->value = gcmall(MAX_BUFFER_SIZE * sizeof(char *));
  input->flag = FLAG_NONE;
  input->line = 0;
  input->row = 0;
  input->next = gcmall(MAX_BUFFER_SIZE * sizeof(char *));

  return input;
}
