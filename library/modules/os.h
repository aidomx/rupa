#pragma once
#if defined(RUPA_PACKAGE_H)

/* Initialize the os module and register its functions */
struct InterpreterResult stdOsInit(struct Node *node, int id,
                                   struct RuntimeEnv *env, struct Error *error);

#endif
