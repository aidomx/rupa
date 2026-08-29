#include <rupa.h>

/* Forward declarations for module initializers */
extern InterpreterResult stdOsInit(Node *node, int id, RuntimeEnv *env,
                                   Error *error);
extern InterpreterResult stdIoInit(Node *node, int id, RuntimeEnv *env,
                                   Error *error);
extern InterpreterResult stdMathInit(Node *node, int id, RuntimeEnv *env,
                                     Error *error);
extern InterpreterResult stdStringInit(Node *node, int id, RuntimeEnv *env,
                                       Error *error);
extern InterpreterResult stdJsonInit(Node *node, int id, RuntimeEnv *env,
                                     Error *error);

/* Module registry */
typedef struct {
  const char *name;
  InterpreterResult (*init)(Node *node, int id, RuntimeEnv *env, Error *error);
} StdModuleEntry;

static StdModuleEntry stdlib_modules[] = {{"os", stdOsInit},
                                         {"io", stdIoInit},
                                         {"math", stdMathInit},
                                         {"string", stdStringInit},
                                         {"json", stdJsonInit},
                                         {NULL, NULL}};

/* Initialize all standard modules and register them in the environment */
void stdlibInit(RuntimeEnv *env) {
  if (!env)
    return;

  for (int i = 0; stdlib_modules[i].name != NULL; i++) {
    /* Create the module object */
    InterpreterResult result = stdlib_modules[i].init(NULL, -1, env, NULL);
    if (result.flow == FLOW_NORMAL) {
      semSet(env, stdlib_modules[i].name, result.value);
    }
  }
}

/* Get a standard module by name */
bool stdlibGetModule(const char *name, RuntimeValue *out) {
  if (!name)
    return false;

  for (int i = 0; stdlib_modules[i].name != NULL; i++) {
    if (strcmp(stdlib_modules[i].name, name) == 0) {
      InterpreterResult result = stdlib_modules[i].init(NULL, -1, NULL, NULL);
      if (result.flow == FLOW_NORMAL && out) {
        *out = result.value;
        return true;
      }
    }
  }
  return false;
}
