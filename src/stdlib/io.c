#include <rupa.h>
#include <stdlib.h>

/* Declared in src/editor/display/terminal.c */
extern struct termios rupaterm;

/* Track whether raw mode was ever enabled */
static bool rawModeActive = false;

/* ==================== io.input(prompt?) ==================== */
static InterpreterResult ioInput(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  /* Print prompt if provided */
  if (argc >= 1 && argv[0].type == VALUE_STRING && argv[0].as.string) {
    printf("%s", argv[0].as.string);
    fflush(stdout);
  }

  /* Temporarily disable raw mode so getline works */
  if (rawModeActive)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rupaterm);

  /* Read line from stdin */
  char *line = NULL;
  size_t capacity = 0;
  ssize_t len = getline(&line, &capacity, stdin);

  /* Restore raw mode if it was active before */
  if (rawModeActive)
    enableRawMode();

  if (len < 0) {
    free(line);
    return resultNormal(valueNull());
  }

  /* Strip trailing newline */
  if (len > 0 && line[len - 1] == '\n')
    line[len - 1] = '\0';

  RuntimeValue result = valueString(line);
  free(line);
  return resultNormal(result);
}

/* ==================== io.toNumber(str) ==================== */
static InterpreterResult ioToNumber(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return resultNormal(valueNull());

  const char *str = argv[0].as.string;

  /* Try integer first */
  char *end;
  long intval = strtol(str, &end, 10);
  if (*end == '\0')
    return resultNormal(valueNumber((int)intval));

  /* Try float */
  double dblval = strtod(str, &end);
  if (*end == '\0')
    return resultNormal(valueDecimal(dblval));

  return resultNormal(valueNull());
}

/* ==================== Module init ==================== */
static void addEntry(struct RuntimeObjectEntry **head, const char *name,
                     NativeFn fn, int paramCount) {
  struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
  e->key = strdup(name);
  e->value = valueNativeFunction(name, fn, paramCount);
  e->next = *head;
  *head = e;
}

/* Called once to mark raw mode as active (from REPL) */
void stdIoSetRawMode(bool active) {
  rawModeActive = active;
}

InterpreterResult stdIoInit(Node *node, int id, RuntimeEnv *env,
                            Error *error) {
  struct RuntimeObjectEntry *entries = NULL;

  addEntry(&entries, "input", ioInput, 0);
  addEntry(&entries, "toNumber", ioToNumber, 1);

  return resultNormal(valueObject(entries));
}
