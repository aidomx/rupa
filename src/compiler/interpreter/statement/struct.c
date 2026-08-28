#include <rupa.h>

InterpreterResult interpretStruct(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  (void)error;
  if (!node || !ast || ast->type != NODE_STRUCT_DECL)
    return resultNormal(valueNull());

  /* Struct declarations register a type name in the environment.
   * The actual field layout is described by the body block's annotations,
   * but at runtime we only need to know the type exists so that
   * annotation validation can accept it. */
  const char *name = NULL;
  if (ast->asStruct.name >= 0 && ast->asStruct.name < node->length) {
    AstNode *n = &node->ast[ast->asStruct.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  if (name)
    semDeclare(env, name, "struct");

  return resultNormal(valueNull());
}
