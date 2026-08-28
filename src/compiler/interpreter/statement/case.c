#include <rupa.h>

InterpreterResult interpretCase(Node *node, AstNode *ast, RuntimeEnv *env,
                                Error *error) {
  if (!node || !ast || ast->type != NODE_CASE) return resultNormal(valueNull());

  InterpreterResult subject = interpretNode(node, ast->asCase.subject, env, error);
  if (subject.flow != FLOW_NORMAL) return subject;

  for (int i = 0; i < ast->asCase.length; i++) {
    struct AstCaseEntry *entry = &ast->asCase.entries[i];

    if (entry->wildcard || entry->pattern < 0)
      return interpretNode(node, entry->body, env, error);

    InterpreterResult pattern = interpretNode(node, entry->pattern, env, error);
    if (pattern.flow != FLOW_NORMAL) return pattern;

    if (valueEquals(subject.value, pattern.value))
      return interpretNode(node, entry->body, env, error);
  }

  return resultNormal(valueNull());
}
