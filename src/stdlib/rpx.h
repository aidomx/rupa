#pragma once
#if defined(RUPA_PACKAGE_H)

/* Dynamic template .rpx (design/rupa_rpx.txt) — shared declarations
 * between rpx.c (modules resources/render) and member.c (__accessor). */

/* Shared read helper (defined in fsbase.c, dipakai rpx.c): baca file
 * sebagai VALUE_STRING. Error dicatat bila gagal. */
InterpreterResult fsbaseReadString(const char *path, Error *error);

/* __accessor handler untuk r.id.<name> (dipanggil interpretMember). */
InterpreterResult rpxResourceAccess(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error);

/* Module initializers (diregistrasi di stdlib.c). */
InterpreterResult stdResourcesInit(struct Node *node, int id,
                                   struct RuntimeEnv *env,
                                   struct Error *error);
InterpreterResult stdRenderInit(struct Node *node, int id,
                                struct RuntimeEnv *env, struct Error *error);

#endif
