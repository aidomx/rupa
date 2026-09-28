#include <rupa.h>
#include "execute_internal.h"

/* execute_call.c — resolve callee + pemanggilan: native function,
 * closure AST (VALUE_FUNCTION), IRFunction module, fallback print.
 * Unit hasil split execute.c:
 *   execute_machine.c   — core mesin (register temp, get/set)
 *   execute_function.c  — execFunction (loop blok + handler instruksi)
 *   execute.c           — entry point modul
 */

/* Pencarian IRFunction by name di module. */
IRFunction *findFunction(IRModule *module, const char *name) {
  if (!module || !name) return NULL;
  for (IRFunction *f = module->first_function; f; f = f->next)
    if (f->name && !strcmp(f->name, name)) return f;
  return NULL;
}

/* Nama parameter dari AST (sejajar paramName di function/call.c). */
const char *irParamName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) return a->identifier.name;
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  if (a->type == NODE_ANNOTATION) {
    int name = a->annotation.name;
    if (name >= 0 && name < node->length && node->ast[name].type == NODE_IDENTIFIER)
      return node->ast[name].identifier.name;
  }
  return NULL;
}

RuntimeValue execCall(IRMachine *m, IRValue *callee, IRValue **args, size_t count) {
  const char *name = callee && callee->data.name ? callee->data.name : NULL;

  /* 1) Native / function dari env (sejajar interpretCall). */
  RuntimeValue fnValue = valueNull();
  if (callee && callee->canon)
    semGetCanon(m->env, callee->canon, callee->nameHash, &fnValue);
  else if (name)
    semGet(m->env, name, &fnValue);
  /* Callee bisa juga berupa register temp (mis. hasil member_get). */
  if (fnValue.type == VALUE_NULL) fnValue = machineGet(m, callee);

  if (fnValue.type == VALUE_NATIVE_FUNCTION && fnValue.as.nativeFunc) {
    struct RuntimeNativeFunction *nf = fnValue.as.nativeFunc;
    int argc = (int)count + (nf->hasReceiver ? 1 : 0);
    RuntimeValue *argv = calloc(argc > 0 ? (size_t)argc : 1, sizeof(RuntimeValue));
    if (!argv) return valueNull();
    int offset = 0;
    if (nf->hasReceiver) {
      argv[0] = nf->receiver ? *nf->receiver : valueNull();
      offset = 1;
    }
    for (size_t i = 0; i < count; i++)
      argv[i + offset] = machineGet(m, args[i]);
    InterpreterResult result = nf->func(argc, argv, m->env, m->error);
    free(argv);
    /* FLOW_ERROR dari native (TypeError, MemoryError, ...) = fatal:
     * hentikan seluruh eksekusi via halt flag. */
    if (result.flow == FLOW_ERROR) machineHalt(m);
    return result.value;
  }

  /* 2) VALUE_FUNCTION (closure AST — hasil import/namespace): jalankan
   * body via interpretNode di env lokal berisi param, sejajar
   * interpretCall. Frame env tidak di-free (sejajar interpreter). */
  if (fnValue.type == VALUE_FUNCTION && fnValue.as.function) {
    RuntimeFunction *function = fnValue.as.function;
    RuntimeEnv *local = semCreateEnv(function->closure);
    if (!local) return valueNull();
    int argc = (int)count;
    if (argc > function->paramLength) argc = function->paramLength;
    for (int i = 0; i < argc; i++) {
      const char *pname = irParamName(function->node, function->params[i]);
      if (pname) semSet(local, pname, machineGet(m, args[i]));
    }
    InterpreterResult r = interpretNode(function->node, function->body, local, m->error);
    /* Nama fungsi untuk pesan error kontrak return-type. */
    const char *fname = NULL;
    if (function->name >= 0 && function->name < function->node->length) {
      AstNode *nn = &function->node->ast[function->name];
      if (nn->type == NODE_IDENTIFIER) fname = nn->identifier.name;
      else if (nn->type == NODE_LITERAL_ID) fname = nn->string.value;
    }
    /* void enforcement (sejajar IR_RETURN & interpretCall):
     * `foo(): void { return v }` dengan v non-null = TypeError fatal. */
    if (r.flow == FLOW_RETURN && function->returnType >= 0) {
      char typeName[256];
      if (formatAstTypeName(function->node, function->returnType, typeName, sizeof(typeName))) {
        if (!strcmp(typeName, "void")) {
          /* void: nilai apa pun (non-null) = TypeError fatal. */
          if (r.value.type != VALUE_NULL) {
            if (m->error) {
              static char message[256];
              snprintf(message, sizeof(message), "function '%s' is void and cannot return a value",
                       fname ? fname : "?");
              addError(m->error, (ErrorInfo){.code = "TypeError",
                                             .message = message,
                                             .line = 0,
                                             .row = 0,
                                             .type = ERR_TYPE_MISMATCH});
            }
            machineHalt(m);
          }
        } else if (r.value.type != VALUE_NULL) {
          /* Kontrak return-type non-void: sejajar IR_CHECK di buildReturn —
           * pesan error presisi via analyzerCheckReturnType. */
          if (!analyzerCheckReturnType(fname, typeName, r.value, m->error)) {
            machineHalt(m);
            return valueNull(); /* jangan propagasi nilai yang gagal kontrak */
          }
        }
      }
    }
    if (r.flow == FLOW_ERROR) machineHalt(m);
    return r.value;
  }

  /* 3) IRFunction di module ini (fungsi hasil rewrite). */
  if (name) {
    IRFunction *fn = findFunction(m->module, name);
    if (fn) {
      RuntimeValue *argv = NULL;
      if (count > 0) {
        argv = calloc(count, sizeof(RuntimeValue));
        if (!argv) return valueNull();
        for (size_t i = 0; i < count; i++)
          argv[i] = machineGet(m, args[i]);
      }
      RuntimeValue out = execFunction(m, fn, argv, (int)count);
      free(argv);
      return out;
    }
  }

  /* 4) Fallback builtin IR: print (bila env tidak punya binding print).
   * Render via print engine (print_format.c) — sejajar interpreter:
   * pola 2 multi-arg + pola 3 format + pola 4 stream target. */
  if (name && !strcmp(name, "print")) {
    RuntimeValue *argv = NULL;
    if (count > 0) {
      argv = malloc(sizeof(RuntimeValue) * count);
      if (!argv) return valueNull();
      for (size_t i = 0; i < count; i++)
        argv[i] = machineGet(m, args[i]);
    }
    bool streamed = false;
    printRenderArgs(argv, (int)count, m->env, m->error, stdout, false, &streamed);
    free(argv);
    return valueNull();
  }

  if (m->error && name) {
    static char message[256];
    snprintf(message, sizeof(message), "'%s' is not a function and cannot be called", name);
    addError(m->error, (ErrorInfo){.code = "TypeError",
                                   .message = message,
                                   .line = 0,
                                   .row = 0,
                                   .type = ERR_INVALID_CALL});
  }
  return valueNull();
}
