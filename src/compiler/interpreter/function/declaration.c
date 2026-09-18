#include <rupa.h>

/* Validasi return terhadap return-type annotation:
 * `foo(): void { return 1 }` = error (void tidak mengembalikan nilai).
 * Return tanpa nilai pada fungsi void sah; fungsi tanpa anotasi bebas. */
static bool validateReturnType(Node *node, AstNode *ast, InterpreterResult r,
                               Error *error) {
  if (ast->function.returnType < 0 || ast->function.returnType >= node->length)
    return true;
  if (r.flow != FLOW_RETURN) return true; /* jatuh keluar body = null, OK */

  char typeName[256];
  if (!formatAstTypeName(node, ast->function.returnType, typeName, sizeof(typeName)))
    return true;
  if (strcmp(typeName, "void")) return true; /* tipe lain belum di-enforce */

  /* Body void boleh `return` telanjang (expression -1 / null). */
  if (r.value.type == VALUE_NULL) return true;

  if (error) {
    static char message[256];
    snprintf(message, sizeof(message),
             "void function cannot return a value");
    addError(error, (ErrorInfo){.code = "TypeError",
                                .message = message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_TYPE_MISMATCH});
  }
  return false;
}

InterpreterResult interpretFunction(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  (void)env;
  if (!node || !ast || ast->type != NODE_FUNCTION_DECL)
    return resultNormal(valueNull());

  /* `foo(): void { return 1 }` — cek langsung di body saat deklarasi
   * (statement-level), sejajar jalur IR yang menandai IR_RETURN. */
  if (ast->function.body >= 0 && ast->function.body < node->length) {
    AstNode *body = &node->ast[ast->function.body];
    if (body->type == NODE_BLOCK) {
      for (int i = 0; i < body->block.length; i++) {
        int sid = body->block.statements[i];
        if (sid < 0 || sid >= node->length) continue;
        AstNode *s = &node->ast[sid];
        if (s->type == NODE_RETURN && s->asReturn.explicitReturn &&
            s->asReturn.expression >= 0) {
          InterpreterResult probe = resultFlow(FLOW_RETURN, valueNumber(1));
          if (!validateReturnType(node, ast, probe, error))
            return resultFlow(FLOW_ERROR, valueNull());
        }
      }
    }
  }

  RuntimeFunction *function = gccalloc(1, sizeof(*function));
  if (!function) return resultFlow(FLOW_ERROR, valueNull());

  function->node = node;
  function->name = ast->function.name;
  function->params = ast->function.params;
  function->paramLength = ast->function.paramLength;
  function->body = ast->function.body;
  function->returnType = ast->function.returnType;
  function->closure = env;

  if (function->name >= 0 && function->name < node->length) {
    AstNode *name = &node->ast[function->name];
    if (name->type == NODE_IDENTIFIER && name->identifier.name)
      semSet(env, name->identifier.name, valueFunction(function));
  }

  return resultNormal(valueFunction(function));
}
