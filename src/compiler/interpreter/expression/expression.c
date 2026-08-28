#include <rupa.h>

InterpreterResult interpretLiteral(Node *, AstNode *);
InterpreterResult interpretIdentifier(Node *, AstNode *, RuntimeEnv *);
InterpreterResult interpretBinary(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretArray(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretObject(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretMember(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretAsync(Node *, AstNode *, RuntimeEnv *, Error *);
InterpreterResult interpretAwait(Node *, AstNode *, RuntimeEnv *, Error *);

InterpreterResult interpretExpression(Node *node, int id, RuntimeEnv *env,
                                      Error *error) {
  if (!node || id < 0 || id >= node->length)
    return resultNormal(valueNull());
  AstNode *ast = &node->ast[id];
  switch (ast->type) {
  case NODE_NUMBER:
  case NODE_DECIMAL:
  case NODE_BOOLEAN:
  case NODE_STRING:
  case NODE_NULLABLE:
    return interpretLiteral(node, ast);
  case NODE_IDENTIFIER:
  case NODE_LITERAL_ID:
    return interpretIdentifier(node, ast, env);
  case NODE_BINARY:
    return interpretBinary(node, ast, env, error);
  case NODE_ARRAY:
    return interpretArray(node, ast, env, error);
  case NODE_OBJECT:
    return interpretObject(node, ast, env, error);
  case NODE_MEMBER:
    return interpretMember(node, ast, env, error);
  case NODE_CALL:
    return interpretCall(node, ast, env, error);
  case NODE_UPDATE:
    return interpretUpdate(node, ast, env, error);
  case NODE_SUBSCRIPT:
    return interpretSubscript(node, ast, env, error);
  case NODE_ASYNC:
    return interpretAsync(node, ast, env, error);
  case NODE_AWAIT:
    return interpretAwait(node, ast, env, error);
  case NODE_FALLBACK: {
    InterpreterResult primary =
        interpretExpression(node, ast->fallback.primary, env, error);
    return valueTruthy(primary.value)
               ? primary
               : interpretExpression(node, ast->fallback.fallback, env, error);
  }
  case NODE_THEN: {
    InterpreterResult condition =
        interpretExpression(node, ast->then.condition, env, error);
    return valueTruthy(condition.value)
               ? interpretExpression(node, ast->then.result, env, error)
               : resultNormal(valueNull());
  }
  default:
    return resultNormal(valueNull());
  }
}
