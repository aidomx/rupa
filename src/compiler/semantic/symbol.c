#include <rupa.h>

RuntimeEnv *semCreateEnv(RuntimeEnv *parent) {
  RuntimeEnv *env = calloc(1, sizeof(*env));
  if (env)
    env->parent = parent;
  return env;
}

RuntimeBinding *semFindLocal(RuntimeEnv *env, const char *name) {
  if (!env || !name) return NULL;
  for (RuntimeBinding *b = env->bindings; b; b = b->next)
    if (!strcmp(b->name, name)) return b;
  return NULL;
}

RuntimeBinding *semFind(RuntimeEnv *env, const char *name) {
  for (; env; env = env->parent)
    for (RuntimeBinding *b = env->bindings; b; b = b->next)
      if (!strcmp(b->name, name)) return b;
  return NULL;
}

bool semDeclare(RuntimeEnv *env, const char *name, const char *type) {
  if (!env || !name) return false;

  RuntimeBinding *b = semFindLocal(env, name);
  if (b) {
    if (type && !b->type) b->type = strdup(type);
    return true;
  }

  b = calloc(1, sizeof(*b));
  if (!b) return false;

  b->name = strdup(name);
  b->type = type ? strdup(type) : NULL;
  b->value = valueNull();
  b->next = env->bindings;
  env->bindings = b;
  return true;
}

void semSet(RuntimeEnv *env, const char *name, RuntimeValue value) {
  if (!env || !name) return;

  RuntimeBinding *b = semFindLocal(env, name);
  if (!b) {
    if (!semDeclare(env, name, NULL)) return;
    b = semFindLocal(env, name);
  }
  if (b) b->value = value;
}

const char *semType(RuntimeEnv *env, const char *name) {
  RuntimeBinding *b = semFind(env, name);
  return b ? b->type : NULL;
}

bool semGet(RuntimeEnv *env, const char *name, RuntimeValue *out) {
  RuntimeBinding *b = semFind(env, name);
  if (b) {
    if (out) *out = b->value;
    return true;
  }
  return false;
}
