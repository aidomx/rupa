#ifndef RUPA_IR_EXECUTE_INTERNAL_H
#define RUPA_IR_EXECUTE_INTERNAL_H

#include <rupa.h>

/*
 * execute_internal.h — kontrak internal antar-unit mesin eksekusi IR
 * (rules.md: modular, jangan membengkak). execute.c dipecah menjadi:
 *
 *   execute_machine.c   — IRMachine: register temp, get/set, eval binary
 *   execute_call.c      — execCall (native/closure/IRFunction) + lookup
 *   execute_function.c  — execFunction (loop blok + handler instruksi)
 *   execute.c           — entry point modul (executeIR + varian error)
 *
 * Semua unit include <rupa.h> + header ini. Tidak ada state global
 * selain g_event_loop (execute.c, sudah extern di file asal).
 */

/* Mesin eksekusi IRModule — satu instance per frame panggilan. */
typedef struct IRMachine {
  IRModule *module;
  RuntimeEnv *env;
  Error *error;
  Node *astRef; /* pool AST untuk trampoline IR_INTERP */
  /* Register temp: hasil IRInstruction dengan result IR_VALUE_TEMP.
   * Tanpa flag stored — temp IR SSA-like selalu ditulis sebelum dibaca
   * di jalur normal; slot lama tidak pernah di-clear per instruksi. */
  RuntimeValue *vals;
  int valLen;
  int valCap;
  /* Flag halt: error fatal (FLOW_ERROR) menghentikan eksekusi. Frame
   * anak berbagi flag mesin root via haltLink agar nested call ikut
   * berhenti. */
  bool halt;
  bool *haltLink;
} IRMachine;

/* ==================== execute_machine.c ==================== */

void machineInit(IRMachine *m, IRModule *ir, RuntimeEnv *env, Error *error, Node *astRef);
void machineHalt(IRMachine *m);
bool machineHalted(const IRMachine *m);
void machineFree(IRMachine *m);
void machineReserve(IRMachine *m, uint32_t id);
RuntimeValue machineGet(IRMachine *m, IRValue *v);
void machineSet(IRMachine *m, IRValue *v, RuntimeValue value);
bool machineTruthy(IRMachine *m, IRValue *v);

/* number 64-bit: number op number tetap number (integer, range 64) —
 * identik dengan numericResult di interpreter (expression/binary.c). */
RuntimeValue irNumericResult(RuntimeValue l, RuntimeValue r, double val);

/* Konversi opcode biner ke RuntimeValue dengan semantik interpretBinary:
 * dua number -> number, campuran -> decimal, string pada IR_ADD -> concat. */
RuntimeValue evalBinaryValue(IROpcode op, RuntimeValue l, RuntimeValue r);

/* ==================== execute_call.c ==================== */

/* Pencarian IRFunction by name di module. */
IRFunction *findFunction(IRModule *module, const char *name);

/* Nama parameter dari AST (sejajar paramName di function/call.c). */
const char *irParamName(Node *node, int id);

RuntimeValue execCall(IRMachine *m, IRValue *callee, IRValue **args, size_t count);

/* ==================== execute_function.c ==================== */

/* Eksekusi satu IRFunction: frame env baru, param di-bind by name,
 * lalu jalan blok demi blok mengikuti terminator. */
RuntimeValue execFunction(IRMachine *m, IRFunction *fn, RuntimeValue *args, int argc);

#endif /* RUPA_IR_EXECUTE_INTERNAL_H */
