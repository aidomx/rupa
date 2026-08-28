#include <rupa.h>

void *createDebug(int capacity) {
  if (capacity <= 0)
    return NULL;

  Debug *debug = gccalloc(capacity, sizeof(Debug));
  if (!debug)
    fprintf(stderr, "Memory for allocation Debug is failed!\n");

  debug->capacity = capacity;
  debug->enabled = false;
  debug->length = 0;
  debug->line = 0;
  debug->message = NULL;
  debug->type = DEBUG_NONE;
  debug->value.boolean = false;
  debug->next = NULL;
  return debug;
}

void setdebug(Debug *debug) {
  if (!debug->enabled)
    return;

  switch (debug->type) {
  case DEBUG_BOOLEAN:
    fprintf(stderr, "%s = %s\n", debug->message,
            debug->value.boolean ? "true" : "false");
    break;

  case DEBUG_NUMBER:
    fprintf(stderr, "%s = %d\n", debug->message, debug->value.number);
    break;

  case DEBUG_STRING:
    fprintf(stderr, "%s = %s\n", debug->message, debug->message);
    break;

  case DEBUG_CHAR:
    fprintf(stderr, "%s = '%c'\n", debug->message, debug->value.character);
    break;

  case DEBUG_POINTER:
    fprintf(stderr, "%s = %p\n", debug->message, debug->value.pointer);
    break;

  case DEBUG_SIZEOF:
    fprintf(stderr, "%s = %zu bytes\n", debug->message, debug->value.size);
    break;

  default:
    if (debug->message)
      fprintf(stderr, "%s\n", debug->message);
    break;
  }
}
