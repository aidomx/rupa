#pragma once
// proses eksekusi hasil dari rewrite IR
#if defined(RUPA_PACKAGE_H)

/**
 * Mengeksekusi IRModule hasil rewrite().
 *
 * Fungsi "main" (hasil rewrite NODE_PROGRAM) dijalankan di environment
 * baru yang sudah terisi stdlib + builtins. Parameter ast adalah pool
 * AST sumber, dibutuhkan trampoline IR_INTERP (NODE_MOD import/export/
 * namespace dievaluasi via interpretNode). Nama executeIR dipakai agar
 * tidak bentrok dengan execute(const char*) milik prompt.
 */
void executeIR(IRModule *ir, Node *ast);

/* Variant dengan Error eksternal: caller mengirim Error-nya sendiri,
 * sehingga status error (mis. IR_CHECK gagal) terlihat oleh caller.
 * Return: error->size > 0. */
int executeIRError(IRModule *ir, Node *ast, Error *error);

/* Variant dengan hook register env tambahan (mis. test helpers assertEq).
 * Return 0 = sukses, 1 = error runtime atau assertion gagal. */
int executeIRErrorWithEnv(IRModule *ir, Node *ast, Error *error,
                          void (*registerEnv)(RuntimeEnv *));

/* Register test helpers (assertEq, assert) — dipakai harness --test-irexec. */
void executeIRRegisterHelpers(RuntimeEnv *env);

#endif
