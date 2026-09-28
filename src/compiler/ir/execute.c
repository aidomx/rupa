#include <rupa.h>
#include "execute_internal.h"

/* ============================================================
 * execute.c — IR -> interpreter (entry point modul)
 *
 * Mesin eksekusi IRModule hasil rewrite.c. Instruksi IR dievaluasi
 * ke RuntimeValue lalu diteruskan ke semantic layer (semGet/semSet)
 * sehingga hasilnya sejajar dengan menjalankan AST langsung lewat
 * interpretNode:
 *
 *   AST --rewrite--> IRModule --executeIR--> RuntimeValue
 *
 * Model memori:
 *   - IR_VALUE_LOCAL/PARAM/GLOBAL/FUNCTION di-resolve by NAME ke
 *     binding RuntimeEnv (slot IR hanya pembawa nama).
 *   - IRValue temp (IR_VALUE_TEMP) disimpan di register table mesin
 *     per-id (phi-less: temp yang belum di-store terbaca null).
 *   - Konstanta IR_VALUE_CONSTANT dikonversi langsung ke RuntimeValue.
 *
 * Kontrol flow:
 *   - Eksekusi blok linear sampai terminator: IR_RETURN / IR_JUMP /
 *     IR_BRANCH. IR_BRANCH memilih blok lanjutan berdasarkan
 *     valueTruthy kondisi, sesuai semantik valueTruthy() interpreter.
 *
 * Trampoline IR_INTERP:
 *   - Node yang butuh runtime penuh (NODE_MOD: import/export/namespace)
 *     dievaluasi langsung via interpretNode() (deklarasi di eval.h).
 *
 * Unit hasil split (rules.md: modular, jangan membengkak):
 *   execute_internal.h  — kontrak internal + struct IRMachine
 *   execute_machine.c   — core mesin (register temp, get/set, binary)
 *   execute_call.c      — execCall + lookup fungsi
 *   execute_function.c  — execFunction (loop blok + handler instruksi)
 *   execute.c           — entry point modul (file ini)
 * ============================================================ */

/* ==================== Eksekusi modul ==================== */

void executeIR(IRModule *ir, Node *ast) {
  if (!ir) return;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return;

  stdlibInit(env);
  builtinsInit(env);

  /* Event loop untuk async — sejajar runner.c. */
  extern struct EventLoop *g_event_loop;
  g_event_loop = eventLoopCreate();

  Error *error = createError(10);
  IRMachine m;
  machineInit(&m, ir, env, error, ast);

  IRFunction *main = findFunction(ir, "main");
  if (main) execFunction(&m, main, NULL, 0);

  /* Drain event loop sampai semua pending event selesai. */
  for (int i = 0; i < 1000 && eventLoopHasPending(g_event_loop); i++)
    eventLoopRun(ast, g_event_loop, env, error);

  if (error && error->size > 0) printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;

  machineFree(&m);
}

/* Variant dengan Error eksternal — status error terlihat caller. */
int executeIRError(IRModule *ir, Node *ast, Error *error) {
  return executeIRErrorWithEnv(ir, ast, error, NULL);
}

/* Variant dengan hook register env tambahan (mis. test helpers).
 * Status gagal = error runtime ATAU assertion helper gagal. */
int executeIRErrorWithEnv(IRModule *ir, Node *ast, Error *error,
                          void (*registerEnv)(RuntimeEnv *)) {
  if (!ir) return 1;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return 1;

  stdlibInit(env);
  builtinsInit(env);
  if (registerEnv) registerEnv(env);

  extern struct EventLoop *g_event_loop;
  g_event_loop = eventLoopCreate();

  IRMachine m;
  machineInit(&m, ir, env, error, ast);

  IRFunction *main = findFunction(ir, "main");
  if (main) execFunction(&m, main, NULL, 0);

  for (int i = 0; i < 1000 && eventLoopHasPending(g_event_loop); i++)
    eventLoopRun(ast, g_event_loop, env, error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;

  machineFree(&m);
  if (error && error->size > 0) return 1;
  return (testHelperFailures() > 0) ? 1 : 0;
}

/* Register test helpers (assertEq, assert) di env IR machine —
 * menyamakan kemampuan --test-exec. */
void executeIRRegisterHelpers(RuntimeEnv *env) {
  if (!env) return;
  testHelperReset();
  testHelperInit(env);
}
