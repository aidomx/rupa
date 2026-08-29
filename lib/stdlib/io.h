#pragma once
#if defined(RUPA_PACKAGE_H)

/* Initialize the io module and register its functions */
struct InterpreterResult stdIoInit(struct Node *node, int id,
                                   struct RuntimeEnv *env, struct Error *error);

/* Notify io module whether raw mode is active */
void stdIoSetRawMode(bool active);

#endif
