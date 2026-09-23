#pragma once
#if defined(RUPA_PACKAGE_H)

/* new Object(ref?, init?) — design/object.txt (defined in stdlib/object.c).
 * Hook di interpretCall SEBELUM memory/instance hook: callee "new" dengan
 * arg pertama literal "Object". Instance punya member methods
 * has/get/set/delete/update/json/text; strict bila kontrak struct
 * (stamp __type) atau ref yang sudah distamp. */

InterpreterResult objectNewCall(struct Node *node, struct AstNode *ast,
                                struct RuntimeEnv *env, struct Error *error,
                                bool *handled);

/* Member call dispatch (interpretCall): raw args supaya identifier posisi
 * key yang bukan variabel dipakai sebagai nama field —
 * people.set(name, "anggi"). */
InterpreterResult objectMemberCall(struct Node *node, struct AstNode *ast,
                                   struct RuntimeEnv *env, struct Error *error,
                                   bool *handled);

/* Instance new Object()? (entry "__object" boolean). */
bool objectIsInstance(RuntimeValue obj);

/* Member method dispatch untuk instance (dipanggil interpretMember):
 * has/get/set/delete/update/json/text → NativeFn, NULL bila bukan. */
NativeFn objectMemberFn(const char *name);

/* Tulis field ke instance (dipakai interpretMemberAssign): strict layout
 * check bila instance distamp __type + two-way sync ke ref. Return false
 * + error bila ditolak (unknown field / tipe salah / meta "__*"). */
bool objectMemberWrite(RuntimeValue inst, const char *key, RuntimeValue val,
                       struct RuntimeEnv *env, struct Error *error);

#endif
