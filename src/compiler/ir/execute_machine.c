#include <rupa.h>
#include "execute_internal.h"

/* execute_machine.c — core mesin eksekusi IR: register temp, get/set
 * nilai, evaluasi binary. Unit hasil split execute.c:
 *   execute_call.c     — execCall + lookup fungsi
 *   execute_function.c — execFunction (loop blok + handler instruksi)
 *   execute.c          — entry point modul
 */

/* ==================== Mesin ==================== */

void machineInit(IRMachine *m, IRModule *ir, RuntimeEnv *env, Error *error, Node *astRef) {
  memset(m, 0, sizeof(*m));
  m->module = ir;
  m->env = env;
  m->error = error;
  m->astRef = astRef;
}

void machineHalt(IRMachine *m) {
  if (m->haltLink)
    *m->haltLink = true;
  else
    m->halt = true;
}

bool machineHalted(const IRMachine *m) {
  return m->haltLink ? *m->haltLink : m->halt;
}

void machineFree(IRMachine *m) {
  if (m->vals) free(m->vals);
  memset(m, 0, sizeof(*m));
}

/* Reserve: hanya grow buffer — slot lama tidak pernah di-clear. Temp
 * IR SSA-like dibaca selalu setelah ditulis; inisialisasi null hanya
 * untuk slot yang baru dialokasi. */
void machineReserve(IRMachine *m, uint32_t id) {
  if ((int)id < m->valLen) return;
  int need = (int)id + 1;
  if (need > m->valCap) {
    int cap = m->valCap ? m->valCap * 2 : 64;
    while (need > cap)
      cap *= 2;
    RuntimeValue *vals = realloc(m->vals, sizeof(RuntimeValue) * cap);
    if (!vals) return;
    m->vals = vals;
    m->valCap = cap;
  }
  for (int i = m->valLen; i < need; i++)
    m->vals[i] = valueNull();
  m->valLen = need;
}

RuntimeValue machineGet(IRMachine *m, IRValue *v) {
  if (!v) return valueNull();

  switch (v->kind) {
  case IR_VALUE_CONSTANT: {
    switch (v->data.constant.kind) {
    case IR_CONST_NUMBER:
      return valueNumber(v->data.constant.as.number);
    case IR_CONST_DECIMAL:
      return valueDecimal(v->data.constant.as.decimal);
    case IR_CONST_BOOLEAN:
      return valueBoolean(v->data.constant.as.boolean != 0);
    case IR_CONST_STRING:
      return valueString(v->data.constant.as.string ? v->data.constant.as.string : "");
    case IR_CONST_NULL:
    default:
      return valueNull();
    }
  }

  case IR_VALUE_TEMP: {
    /* Fast path: temp sudah ada di buffer pada jalur normal (ditulis
     * sebelum dibaca) — bounds check saja, tanpa reserve/clear. */
    if ((int)v->id < m->valLen) return m->vals[v->id];
    machineReserve(m, v->id);
    return valueNull();
  }

  case IR_VALUE_PARAM:
  case IR_VALUE_LOCAL:
  case IR_VALUE_GLOBAL: {
    if (v->data.name) {
      RuntimeValue out;
      /* canon = nama ter-intern yang di-cache di IRValue — lookup loop
       * panas tanpa intern + hash string per iterasi. */
      if (v->canon) {
        if (semGetCanon(m->env, v->canon, v->nameHash, &out)) return out;
      } else if (semGet(m->env, v->data.name, &out)) {
        return out;
      }
    }
    return valueNull();
  }

  case IR_VALUE_FUNCTION:
    /* Referensi fungsi: katakan ke execCall bahwa ini IRFunction.
     * execCall men-resolve ulang by name di module. */
    return valueNull();

  default:
    return valueNull();
  }
}

void machineSet(IRMachine *m, IRValue *v, RuntimeValue value) {
  if (!v) return;

  if (v->kind == IR_VALUE_TEMP) {
    if ((int)v->id >= m->valLen) machineReserve(m, v->id);
    if ((int)v->id < m->valLen) m->vals[v->id] = value;
    return;
  }

  if (v->data.name) {
    if (v->canon)
      semSetCanon(m->env, v->canon, v->nameHash, value);
    else
      semSet(m->env, v->data.name, value);
  }
}

bool machineTruthy(IRMachine *m, IRValue *v) {
  return valueTruthy(machineGet(m, v));
}

/* number 64-bit: number op number tetap number (integer, range 64) —
 * identik dengan numericResult di interpreter (expression/binary.c).
 * l/r adalah operand; val hasil double-nya. */
RuntimeValue irNumericResult(RuntimeValue l, RuntimeValue r, double val) {
  if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER) {
    if (isfinite(val) && floor(val) == val && val >= -(double)LLONG_MAX && val <= (double)LLONG_MAX)
      return valueNumber((long long)val);
    return valueDecimal(val);
  }
  return valueDecimal(val);
}

/* Konversi opcode biner ke RuntimeValue dengan semantik interpretBinary:
 * dua number -> number, campuran -> decimal, string pada IR_ADD -> concat. */
RuntimeValue evalBinaryValue(IROpcode op, RuntimeValue l, RuntimeValue r) {
  bool numericL = l.type == VALUE_NUMBER || l.type == VALUE_DECIMAL;
  bool numericR = r.type == VALUE_NUMBER || r.type == VALUE_DECIMAL;

  double a = l.type == VALUE_DECIMAL ? l.as.decimal : (double)l.as.number;
  double b = r.type == VALUE_DECIMAL ? r.as.decimal : (double)r.as.number;

  switch (op) {
  case IR_ADD: {
    if (numericL && numericR) {
      return irNumericResult(l, r, a + b);
    }
    /* Concat ala interpreter: textOf kiri + textOf kanan — object,
     * array, dan function di-stringify persis seperti interpretBinary. */
    char *ls = valueTextOf(l);
    char *rs = valueTextOf(r);
    size_t n = strlen(ls) + strlen(rs) + 1;
    char *out = malloc(n);
    if (!out) {
      free(ls);
      free(rs);
      return valueNull();
    }
    snprintf(out, n, "%s%s", ls, rs);
    RuntimeValue concat = valueString(out);
    free(ls);
    free(rs);
    free(out);
    return concat;
  }

  case IR_SUB:
    if (numericL && numericR) return irNumericResult(l, r, a - b);
    return valueNull();
  case IR_MUL:
    if (numericL && numericR) return irNumericResult(l, r, a * b);
    return valueNull();
  case IR_DIV:
    if (numericL && numericR && b != 0) return irNumericResult(l, r, a / b);
    return valueNull();
  case IR_MOD:
    if (numericL && numericR && b != 0)
      return l.type == VALUE_NUMBER && r.type == VALUE_NUMBER
                 ? valueNumber(l.as.number % r.as.number)
                 : valueDecimal((double)((int64_t)a % (int64_t)b));
    return valueNull();

  case IR_EQ:
    return valueBoolean(valueEquals(l, r));
  case IR_NE:
    return valueBoolean(!valueEquals(l, r));
  case IR_LT:
    return valueBoolean(numericL && numericR && a < b);
  case IR_LE:
    return valueBoolean(numericL && numericR && a <= b);
  case IR_GT:
    return valueBoolean(numericL && numericR && a > b);
  case IR_GE:
    return valueBoolean(numericL && numericR && a >= b);

  default:
    return valueNull();
  }
}
