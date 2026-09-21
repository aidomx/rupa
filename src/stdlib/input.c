#include <rupa.h>

/* input.c — sistem @input class (design/new_class.txt poin 5).
 *
 * `@input` marker di atas method class MENGAKTIFKAN `main.input: Input`:
 *
 *   @input
 *   handler() {
 *     return {
 *       name: "Input monster name: ",
 *       health: "Input monster health: "
 *     }
 *   }
 *
 * Return value handler = deklarasi strict field yang boleh diinput
 * (key = nama field, value = prompt string). Runtime:
 *
 *   main extends Monster {
 *     construct(input: Input) {
 *       name = input.get("name")     // prompt + baca stdin
 *     }
 *   }
 *
 * `input.get("...")` STRICT: hanya field yang dideklarasikan handler
 * @input yang diterima — selain itu ReferenceError. Object Input
 * sendiri di-bind ke `input` param construct main oleh runtime
 * (classRunConstruct), bukan dibuat user. */

extern InterpreterResult rupaIoInput(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error);

/* ===== Registry spec @input (per run) ===== */
struct InputSpec {
  char *field;
  char *prompt;
  struct InputSpec *next;
};
static struct InputSpec *g_specs = NULL;
static bool g_active = false;

void inputSpecReset(void) {
  g_specs = NULL;
  g_active = false;
}

/* value = return value handler @input (object literal field:prompt).
 * Hanya entry string yang jadi spec (nilai lain diabaikan). */
void inputSpecRegister(RuntimeValue spec) {
  if (spec.type != VALUE_OBJECT || !spec.as.object.entries) return;
  for (struct RuntimeObjectEntry *e = spec.as.object.entries; e; e = e->next) {
    if (!e->key || e->value.type != VALUE_STRING || !e->value.as.string) continue;
    struct InputSpec *s = gccalloc(1, sizeof(*s));
    if (!s) return;
    s->field = gcstrdup(e->key);
    s->prompt = gcstrdup(e->value.as.string);
    s->next = g_specs;
    g_specs = s;
  }
  g_active = true;
}

bool inputSpecActive(void) {
  return g_active && g_specs != NULL;
}

const char *inputSpecPrompt(const char *field) {
  if (!field) return NULL;
  for (struct InputSpec *s = g_specs; s; s = s->next)
    if (!strcmp(s->field, field)) return s->prompt;
  return NULL;
}

/* ===== Runtime Input object ===== */

/* input.get("field") — strict terhadap spec handler @input.
 * Dipanggil via member call `input.get(...)`; receiver = object Input
 * (opaque, identitas tidak dipakai — spec-nya global per run). */
static InterpreterResult inputGet(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (!inputSpecActive()) {
    if (error)
      addError(error, (ErrorInfo){.code = "ReferenceError",
                                  .message = "input is not active (no @input handler)",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_UNDEFINED_VAR});
    return resultFlow(FLOW_ERROR, valueNull());
  }
  /* Pemanggilan member call `input.get("field")`: argv[0] = receiver
   * (object Input), argv[1] = nama field. */
  int fi = (argc >= 2) ? 1 : 0;
  if (argc < 1 + 1 || argv[fi].type != VALUE_STRING || !argv[fi].as.string) {
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = "input.get expects a field name string",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  const char *field = argv[fi].as.string;
  const char *prompt = inputSpecPrompt(field);
  if (!prompt) {
    /* STRICT: field di luar deklarasi @input ditolak. */
    char msg[256];
    snprintf(msg, sizeof(msg), "'%s' is not declared in the @input handler", field);
    if (error)
      addError(error, (ErrorInfo){.code = "ReferenceError",
                                  .message = msg,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_UNDEFINED_VAR});
    return resultFlow(FLOW_ERROR, valueNull());
  }

  /* Prompt + baca satu baris stdin (rupaiIoInput menerima argv[0]
   * string sebagai prompt). */
  RuntimeValue argv2[1] = {valueString(prompt)};
  InterpreterResult r = rupaIoInput(1, argv2, env, error);
  if (r.flow != FLOW_NORMAL) return r;

  return resultNormal(r.value);
}

/* Object Input yang di-bind ke param `input` construct main. */
RuntimeValue inputCreateObject(void) {
  struct RuntimeObjectEntry *entries = NULL;
  RuntimeValue get = valueNativeFunction("get", inputGet, 1);
  RuntimeValue *recv = gcmall(sizeof(RuntimeValue));
  if (recv) {
    *recv = valueNull();
    get.as.nativeFunc->hasReceiver = true;
    get.as.nativeFunc->receiver = recv;
  }
  /* Entri dinamis: member `get` pada object Input harus memicu
   * inputGet, bukan accessor generic object — ditandai entry function
   * biasa; interpretMember menemukan entry ini lebih dulu (lookup
   * valueObjectGet sebelum accessor fallback). */
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  if (!e) return valueObject(NULL);
  e->key = gcstrdup("get");
  e->value = get;
  e->next = NULL;
  entries = e;
  return valueObject(entries);
}
