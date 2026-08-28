#include <rupa.h>

InterpreterResult interpretObject(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  if (!node || !ast || ast->type != NODE_OBJECT)
    return resultNormal(valueNull());

  struct RuntimeObjectEntry *head = NULL;
  struct RuntimeObjectEntry *tail = NULL;

  for (int i = 0; i < ast->object.length; i++) {
    int keyId = ast->object.entries[i].key;
    int valId = ast->object.entries[i].value;

    const char *key = NULL;
    if (keyId >= 0 && keyId < node->length) {
      AstNode *kn = &node->ast[keyId];
      if (kn->type == NODE_IDENTIFIER)
        key = kn->identifier.name;
      else if (kn->type == NODE_LITERAL_ID)
        key = kn->string.value;
    }

    InterpreterResult val = interpretNode(node, valId, env, error);
    if (val.flow != FLOW_NORMAL)
      return val;

    struct RuntimeObjectEntry *entry = calloc(1, sizeof(*entry));
    if (!entry) return resultNormal(valueNull());
    entry->key = key ? strdup(key) : NULL;
    entry->value = val.value;
    entry->next = NULL;

    if (tail) {
      tail->next = entry;
      tail = entry;
    } else {
      head = tail = entry;
    }
  }

  return resultNormal(valueObject(head));
}
