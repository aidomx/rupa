#include <rupa.h>

/* ============================================================
 * execute.c — IR -> interpreter
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
 * ============================================================ */

/* ==================== Mesin ==================== */

typedef struct IRMachine {
  IRModule *module;
  RuntimeEnv *env;
  Error *error;
  Node *astRef; /* pool AST untuk trampoline IR_INTERP */
  /* Register temp: hasil IRInstruction dengan result IR_VALUE_TEMP. */
  RuntimeValue *vals;
  bool *stored;
  int valLen;
  int valCap;
  /* Flag halt: error fatal (FLOW_ERROR) menghentikan eksekusi. Frame
   * anak berbagi flag mesin root via haltLink agar nested call ikut
   * berhenti. */
  bool halt;
  bool *haltLink;
} IRMachine;

static void machineInit(IRMachine *m, IRModule *ir, RuntimeEnv *env, Error *error, Node *astRef) {
  memset(m, 0, sizeof(*m));
  m->module = ir;
  m->env = env;
  m->error = error;
  m->astRef = astRef;
}

static void machineHalt(IRMachine *m) {
  if (m->haltLink)
    *m->haltLink = true;
  else
    m->halt = true;
}

static bool machineHalted(const IRMachine *m) {
  return m->haltLink ? *m->haltLink : m->halt;
}

static void machineFree(IRMachine *m) {
  if (m->vals) free(m->vals);
  if (m->stored) free(m->stored);
  memset(m, 0, sizeof(*m));
}

static void machineReserve(IRMachine *m, uint32_t id) {
  if ((int)id < m->valLen) return;
  int need = (int)id + 1;
  if (need > m->valCap) {
    int cap = m->valCap ? m->valCap * 2 : 64;
    while (need > cap)
      cap *= 2;
    RuntimeValue *vals = realloc(m->vals, sizeof(RuntimeValue) * cap);
    bool *stored = realloc(m->stored, sizeof(bool) * cap);
    if (!vals || !stored) return;
    m->vals = vals;
    m->stored = stored;
    m->valCap = cap;
  }
  for (int i = m->valLen; i < need; i++) {
    m->vals[i] = valueNull();
    m->stored[i] = false;
  }
  m->valLen = need;
}

static RuntimeValue machineGet(IRMachine *m, IRValue *v) {
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

  case IR_VALUE_TEMP:
    machineReserve(m, v->id);
    if ((int)v->id < m->valLen) return m->stored[v->id] ? m->vals[v->id] : valueNull();
    return valueNull();

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

static void machineSet(IRMachine *m, IRValue *v, RuntimeValue value) {
  if (!v) return;

  if (v->kind == IR_VALUE_TEMP) {
    machineReserve(m, v->id);
    if ((int)v->id < m->valLen) {
      m->vals[v->id] = value;
      m->stored[v->id] = true;
    }
    return;
  }

  if (v->data.name) {
    if (v->canon)
      semSetCanon(m->env, v->canon, v->nameHash, value);
    else
      semSet(m->env, v->data.name, value);
  }
}

static bool machineTruthy(IRMachine *m, IRValue *v) {
  return valueTruthy(machineGet(m, v));
}

/* number 64-bit: number op number tetap number (integer, range 64) —
 * identik dengan numericResult di interpreter (expression/binary.c).
 * l/r adalah operand; val hasil double-nya. */
static RuntimeValue irNumericResult(RuntimeValue l, RuntimeValue r, double val) {
  if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER) {
    if (isfinite(val) && floor(val) == val && val >= -(double)LLONG_MAX && val <= (double)LLONG_MAX)
      return valueNumber((long long)val);
    return valueDecimal(val);
  }
  return valueDecimal(val);
}

/* Konversi opcode biner ke RuntimeValue dengan semantik interpretBinary:
 * dua number -> number, campuran -> decimal, string pada IR_ADD -> concat. */
static RuntimeValue evalBinaryValue(IROpcode op, RuntimeValue l, RuntimeValue r) {
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

/* Pencarian IRFunction by name di module. */
static IRFunction *findFunction(IRModule *module, const char *name) {
  if (!module || !name) return NULL;
  for (IRFunction *f = module->first_function; f; f = f->next)
    if (f->name && !strcmp(f->name, name)) return f;
  return NULL;
}

/* Nama parameter dari AST (sejajar paramName di function/call.c). */
static const char *irParamName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *a = &node->ast[id];
  if (a->type == NODE_IDENTIFIER) return a->identifier.name;
  if (a->type == NODE_LITERAL_ID) return a->string.value;
  if (a->type == NODE_ANNOTATION) {
    int name = a->annotation.name;
    if (name >= 0 && name < node->length && node->ast[name].type == NODE_IDENTIFIER)
      return node->ast[name].identifier.name;
  }
  return NULL;
}

static RuntimeValue execFunction(IRMachine *m, IRFunction *fn, RuntimeValue *args, int argc);

static RuntimeValue execCall(IRMachine *m, IRValue *callee, IRValue **args, size_t count) {
  const char *name = callee && callee->data.name ? callee->data.name : NULL;

  /* 1) Native / function dari env (sejajar interpretCall). */
  RuntimeValue fnValue = valueNull();
  if (callee && callee->canon)
    semGetCanon(m->env, callee->canon, callee->nameHash, &fnValue);
  else if (name)
    semGet(m->env, name, &fnValue);
  /* Callee bisa juga berupa register temp (mis. hasil member_get). */
  if (fnValue.type == VALUE_NULL) fnValue = machineGet(m, callee);

  if (fnValue.type == VALUE_NATIVE_FUNCTION && fnValue.as.nativeFunc) {
    struct RuntimeNativeFunction *nf = fnValue.as.nativeFunc;
    int argc = (int)count + (nf->hasReceiver ? 1 : 0);
    RuntimeValue *argv = calloc(argc > 0 ? (size_t)argc : 1, sizeof(RuntimeValue));
    if (!argv) return valueNull();
    int offset = 0;
    if (nf->hasReceiver) {
      argv[0] = nf->receiver ? *nf->receiver : valueNull();
      offset = 1;
    }
    for (size_t i = 0; i < count; i++)
      argv[i + offset] = machineGet(m, args[i]);
    InterpreterResult result = nf->func(argc, argv, m->env, m->error);
    free(argv);
    /* FLOW_ERROR dari native (TypeError, MemoryError, ...) = fatal:
     * hentikan seluruh eksekusi via halt flag. */
    if (result.flow == FLOW_ERROR) machineHalt(m);
    return result.value;
  }

  /* 2) VALUE_FUNCTION (closure AST — hasil import/namespace): jalankan
   * body via interpretNode di env lokal berisi param, sejajar
   * interpretCall. Frame env tidak di-free (sejajar interpreter). */
  if (fnValue.type == VALUE_FUNCTION && fnValue.as.function) {
    RuntimeFunction *function = fnValue.as.function;
    RuntimeEnv *local = semCreateEnv(function->closure);
    if (!local) return valueNull();
    int argc = (int)count;
    if (argc > function->paramLength) argc = function->paramLength;
    for (int i = 0; i < argc; i++) {
      const char *pname = irParamName(function->node, function->params[i]);
      if (pname) semSet(local, pname, machineGet(m, args[i]));
    }
    InterpreterResult r = interpretNode(function->node, function->body, local, m->error);
    /* void enforcement (sejajar IR_RETURN & interpretCall):
     * `foo(): void { return v }` dengan v non-null = TypeError fatal. */
    if (r.flow == FLOW_RETURN && r.value.type != VALUE_NULL && function->returnType >= 0) {
      char typeName[256];
      if (formatAstTypeName(function->node, function->returnType, typeName, sizeof(typeName)) &&
          !strcmp(typeName, "void")) {
        if (m->error)
          addError(m->error, (ErrorInfo){.code = "TypeError",
                                         .message = "void function cannot return a value",
                                         .line = 0,
                                         .row = 0,
                                         .type = ERR_TYPE_MISMATCH});
        machineHalt(m);
      }
    }
    if (r.flow == FLOW_ERROR) machineHalt(m);
    return r.value;
  }

  /* 3) IRFunction di module ini (fungsi hasil rewrite). */
  if (name) {
    IRFunction *fn = findFunction(m->module, name);
    if (fn) {
      RuntimeValue *argv = NULL;
      if (count > 0) {
        argv = calloc(count, sizeof(RuntimeValue));
        if (!argv) return valueNull();
        for (size_t i = 0; i < count; i++)
          argv[i] = machineGet(m, args[i]);
      }
      RuntimeValue out = execFunction(m, fn, argv, (int)count);
      free(argv);
      return out;
    }
  }

  /* 4) Fallback builtin IR: print (bila env tidak punya binding print). */
  if (name && !strcmp(name, "print")) {
    for (size_t i = 0; i < count; i++) {
      if (i) putchar(' ');
      valuePrint(machineGet(m, args[i]));
    }
    return valueNull();
  }

  if (m->error && name) {
    static char message[256];
    snprintf(message, sizeof(message), "'%s' is not a function and cannot be called", name);
    addError(m->error, (ErrorInfo){.code = "TypeError",
                                   .message = message,
                                   .line = 0,
                                   .row = 0,
                                   .type = ERR_INVALID_CALL});
  }
  return valueNull();
}

/* Eksekusi satu IRFunction: frame env baru, param di-bind by name,
 * lalu jalan blok demi blok mengikuti terminator. */
static RuntimeValue execFunction(IRMachine *m, IRFunction *fn, RuntimeValue *args, int argc) {
  RuntimeEnv *local = semCreateEnv(m->env);
  if (!local) return valueNull();

  IRMachine frame;
  machineInit(&frame, m->module, local, m->error, m->astRef);
  /* Berbagi halt flag dengan mesin root agar nested call ikut berhenti. */
  frame.haltLink = m->haltLink ? m->haltLink : &m->halt;

  int bound = argc < (int)fn->param_count ? argc : (int)fn->param_count;
  for (int i = 0; i < bound; i++) {
    IRValue *param = fn->params[i];
    if (param && param->data.name) {
      if (param->canon)
        semSetCanon(local, param->canon, param->nameHash, args[i]);
      else
        semSet(local, param->data.name, args[i]);
    }
  }

  IRBlock *block = fn->first_block;
  while (block) {
    IRBlock *next = NULL;

    for (IRInstruction *i = block->first; i; i = i->next) {
      if (machineHalted(&frame)) break; /* error fatal — hentikan */
      switch (i->op) {
      case IR_CONST:
      case IR_LOAD:
        if (i->result) machineSet(&frame, i->result, machineGet(&frame, i->data.unary.value));
        break;
      case IR_STORE:
        if (i->data.store.target && i->data.store.target->kind == IR_VALUE_FUNCTION) {
          /* Deklarasi fungsi: slot IR_VALUE_FUNCTION = bind fungsi AST
           * ke env sebagai VALUE_FUNCTION (closure AST) — sejajar
           * interpretFunction. Dipakai trampoline interpretNode
           * (callLoader/callHandler async, callback) via semGet. */
          const char *fname = i->data.store.target->data.name;
          for (int ni = 0; ni < m->astRef->length; ni++) {
            AstNode *an = &m->astRef->ast[ni];
            if (an->type != NODE_FUNCTION_DECL) continue;
            const char *dn = NULL;
            if (an->function.name >= 0 && an->function.name < m->astRef->length) {
              AstNode *nn = &m->astRef->ast[an->function.name];
              if (nn->type == NODE_IDENTIFIER) dn = nn->identifier.name;
            }
            if (!dn || strcmp(dn, fname) != 0) continue;
            RuntimeFunction *rf = gccalloc(1, sizeof(*rf));
            if (!rf) break;
            rf->node = m->astRef;
            rf->name = an->function.name;
            rf->params = an->function.params;
            rf->paramLength = an->function.paramLength;
            rf->body = an->function.body;
            rf->returnType = an->function.returnType;
            rf->closure = frame.env;
            machineSet(&frame, i->data.store.target, valueFunction(rf));
            break;
          }
          break;
        }
        /* Const slot (binding-level, bukan nilai): deklarasi const
         * (store ber-flag) selalu menulis + mengunci slot — re-init di
         * setiap iterasi loop / call. Store biasa ke slot terkunci
         * ditolak di sini, sebelum tulis terjadi. Lokasi error = node
         * assignment asal store (payload nodeId). */
        {
          IRValue *target = i->data.store.target;
          const char *name = (target && target->kind != IR_VALUE_TEMP) ? target->data.name : NULL;
          const char *canon = (target && target->kind != IR_VALUE_TEMP) ? target->canon : NULL;
          RuntimeValue value = machineGet(&frame, i->data.store.value);
          bool writeOk;
          if (i->data.store.isConst) {
            if (canon)
              semSetConstCanon(m->env, canon, target->nameHash, value);
            else
              semSetConst(m->env, name, value);
            writeOk = true;
          } else {
            /* semIsConst = semFind penuh tiap store; jalur canon
             * melompati intern + hash string (loop panas). */
            writeOk = !(name && (canon ? semIsConstCanon(m->env, canon, target->nameHash)
                                       : semIsConst(m->env, name)));
          }
          if (writeOk) {
            machineSet(&frame, target, value);
          } else {
            if (frame.error) {
              static char message[256];
              snprintf(message, sizeof(message), "cannot reassign const variable '%s'",
                       name ? name : "?");
              int line = 0, row = 0;
              if (i->data.store.nodeId >= 0 && i->data.store.nodeId < m->astRef->length) {
                line = m->astRef->ast[i->data.store.nodeId].line;
                row = m->astRef->ast[i->data.store.nodeId].row;
              }
              addError(frame.error, (ErrorInfo){.code = "ConstError",
                                                .message = message,
                                                .line = line,
                                                .row = row,
                                                .type = ERR_TYPE_MISMATCH});
            }
            machineHalt(&frame);
            machineFree(&frame);
            return valueNull();
          }
        }
        break;
      case IR_ADD:
      case IR_SUB:
      case IR_MUL:
      case IR_DIV:
      case IR_MOD:
      case IR_EQ:
      case IR_NE:
      case IR_LT:
      case IR_LE:
      case IR_GT:
      case IR_GE:
        if (i->result)
          machineSet(&frame, i->result,
                     evalBinaryValue(i->op, machineGet(&frame, i->data.binary.left),
                                     machineGet(&frame, i->data.binary.right)));
        break;
      case IR_NEG: {
        RuntimeValue v = machineGet(&frame, i->data.unary.value);
        if (v.type == VALUE_NUMBER)
          v = valueNumber(-v.as.number);
        else if (v.type == VALUE_DECIMAL)
          v = valueDecimal(-v.as.decimal);
        if (i->result) machineSet(&frame, i->result, v);
        break;
      }
      case IR_NOT:
        if (i->result)
          machineSet(&frame, i->result,
                     valueBoolean(!valueTruthy(machineGet(&frame, i->data.unary.value))));
        break;
      case IR_MEMBER_GET: {
        RuntimeValue obj = machineGet(&frame, i->data.member_get.object);
        RuntimeValue out = valueNull();
        const char *key = i->data.member_get.member;
        if (obj.type == VALUE_OBJECT) {
          if (valueObjectGet(obj, key, &out)) {
            /* member ditemukan */
          } else {
            /* Enum object (marker "__enum" = nama enum, dari
             * interpretEnum): member tak dikenal = error eksplisit,
             * bukan null senyap (biasanya typo casing). Object biasa
             * tetap mengembalikan null tanpa error. */
            RuntimeValue marker = valueNull();
            if (frame.error && key && strcmp(key, "__enum") != 0 &&
                valueObjectGet(obj, "__enum", &marker) && marker.type == VALUE_STRING) {
              static char message[256];
              snprintf(message, sizeof(message), "'%s' is not a member of enum '%s'", key,
                       marker.as.string ? marker.as.string : "?");
              addError(frame.error, (ErrorInfo){.code = "EnumError",
                                                .message = message,
                                                .line = 0,
                                                .row = 0,
                                                .type = ERR_UNDEFINED_VAR});
              /* Member enum tidak ada = fatal: berhenti eksekusi,
               * selaras IR_CHECK (error sudah ditambahkan). */
              machineHalt(&frame);
              machineFree(&frame);
              return valueNull();
            }
          }
        } else if (obj.type == VALUE_ARRAY && key && !strcmp(key, "length"))
          out = valueNumber(obj.as.array.length);
        else if (obj.type == VALUE_STRING && key && !strcmp(key, "length"))
          out = valueNumber(obj.as.string ? (int)strlen(obj.as.string) : 0);
        else if (obj.type == VALUE_PTR) {
          /* Struct handle ptr (C3): field access via layout offset.
           * Registry v3 menyimpan nama struct-nya. Handle non-struct
           * (pin tanpa type) tetap ditolak seperti sebelumnya. */
          bool fatal = false;
          RuntimeValue fieldOut = valueNull();
          if (obj.as.ptr && memoryMemberGet(obj.as.ptr, key, &fieldOut, frame.error, &fatal)) {
            out = fieldOut;
          } else if (!fatal && obj.as.ptr && !gcregtype(obj.as.ptr) && frame.error) {
            /* pin() sengaja mengembalikan handle opaque tanpa operasi
             * pointer di fase ini (lihat design/pointer.txt, keputusan #3:
             * "POINTER SYNTAX — DITUNDA"). Tanpa cabang ini, field access
             * pada handle pin diam-diam jatuh ke valueNull() di bawah,
             * kelihatan seperti nilainya "hilang" alih-alih ditolak. */
            static char message[256];
            snprintf(message, sizeof(message),
                     "cannot access property '%s' on a pin handle — pointer dereference is not "
                     "implemented yet",
                     key ? key : "?");
            addError(frame.error, (ErrorInfo){.code = "TypeError",
                                              .message = message,
                                              .line = 0,
                                              .row = 0,
                                              .type = ERR_TYPE_MISMATCH});
          }
        } else if (frame.error && key) {
          static char message[256];
          snprintf(message, sizeof(message), "cannot access property '%s' on value of type '%s'",
                   key, valueTypeName(obj.type));
          addError(frame.error, (ErrorInfo){.code = "TypeError",
                                            .message = message,
                                            .line = 0,
                                            .row = 0,
                                            .type = ERR_TYPE_MISMATCH});
        }
        if (i->result) machineSet(&frame, i->result, out);
        break;
      }
      case IR_MEMBER_SET: {
        RuntimeValue obj = machineGet(&frame, i->data.member_set.object);
        RuntimeValue val = machineGet(&frame, i->data.member_set.value);
        if (obj.type == VALUE_OBJECT) {
          /* Instance new Object() (design/object.txt): strict layout
           * check + two-way sync ke ref via objectMemberWrite. */
          if (objectIsInstance(obj) && i->data.member_set.member) {
            if (!objectMemberWrite(obj, i->data.member_set.member, val, frame.env, frame.error)) {
              machineHalt(&frame);
              machineFree(&frame);
              return valueNull();
            }
            if (i->result) machineSet(&frame, i->result, val);
            break;
          }
          /* Enum object bersifat konstanta — tulis member ditolak. */
          RuntimeValue marker = valueNull();
          if (frame.error && valueObjectGet(obj, "__enum", &marker) &&
              marker.type == VALUE_STRING) {
            static char message[256];
            snprintf(message, sizeof(message),
                     "cannot assign member '%s' on enum '%s' — enum members are constants",
                     i->data.member_set.member ? i->data.member_set.member : "?",
                     marker.as.string ? marker.as.string : "?");
            addError(frame.error, (ErrorInfo){.code = "EnumError",
                                              .message = message,
                                              .line = 0,
                                              .row = 0,
                                              .type = ERR_TYPE_MISMATCH});
            machineHalt(&frame);
            machineFree(&frame);
            return valueNull();
          } else {
            valueObjectSet(&obj, i->data.member_set.member, val);
          }
        } else if (obj.type == VALUE_PTR) {
          /* Struct handle ptr (C3): field write via layout offset.
           * Handle non-struct (pin) tetap ditolak eksplisit. */
          bool fatal = false;
          if (obj.as.ptr &&
              memoryMemberSet(obj.as.ptr, i->data.member_set.member, val, frame.error, &fatal)) {
            if (fatal) machineHalt(&frame);
          } else if (fatal) {
            machineHalt(&frame);
          } else if (frame.error) {
            /* pin() belum mendukung dereference — tolak eksplisit. */
            static char message[256];
            snprintf(message, sizeof(message),
                     "cannot assign property '%s' on a pin handle — pointer dereference is not "
                     "implemented yet",
                     i->data.member_set.member ? i->data.member_set.member : "?");
            addError(frame.error, (ErrorInfo){.code = "TypeError",
                                              .message = message,
                                              .line = 0,
                                              .row = 0,
                                              .type = ERR_TYPE_MISMATCH});
          }
        } else if (frame.error) {
          static char message[256];
          snprintf(message, sizeof(message), "cannot assign property '%s' on value of type '%s'",
                   i->data.member_set.member ? i->data.member_set.member : "?",
                   valueTypeName(obj.type));
          addError(frame.error, (ErrorInfo){.code = "TypeError",
                                            .message = message,
                                            .line = 0,
                                            .row = 0,
                                            .type = ERR_TYPE_MISMATCH});
        }
        machineSet(&frame, i->data.member_set.object, obj);
        break;
      }
      case IR_STRSLOT_GET: {
        /* Read-through string slot (design/str_memory.txt). */
        RuntimeValue ptr = machineGet(&frame, i->data.strslot_get.pointer);
        RuntimeValue out = valueNull();
        bool fatal = false;
        if (ptr.type == VALUE_PTR && ptr.as.ptr)
          memoryStringSlotRead(ptr.as.ptr, &out, frame.error);
        else if (frame.error && ptr.type != VALUE_NULL) {
          addError(frame.error, (ErrorInfo){.code = "TypeError",
                                            .message = "string slot read on non-handle",
                                            .line = 0,
                                            .row = 0,
                                            .type = ERR_TYPE_MISMATCH});
          machineHalt(&frame);
        }
        (void)fatal;
        if (i->result) machineSet(&frame, i->result, out);
        break;
      }
      case IR_STRSLOT_SET: {
        /* Write-through string slot: string biasa -> gcstrdup; ptr tanpa
         * tipe (dupl) -> pointer pindah ke slot. Gagal tulis (handle
         * bukan string slot / RHS ptr typed lain) BUKAN error —
         * irStore berikutnya di jalur normal melakukan rebind. */
        RuntimeValue ptr = machineGet(&frame, i->data.strslot_set.pointer);
        RuntimeValue val = machineGet(&frame, i->data.strslot_set.value);
        if (ptr.type == VALUE_PTR && ptr.as.ptr)
          memoryStringSlotWrite("", ptr.as.ptr, val, frame.error);
        break;
      }
      case IR_INDEX_GET: {
        RuntimeValue arr = machineGet(&frame, i->data.index_get.array);
        RuntimeValue idx = machineGet(&frame, i->data.index_get.index);
        RuntimeValue out = valueNull();
        if (arr.type == VALUE_ARRAY && idx.type == VALUE_NUMBER) {
          int n = (int)idx.as.number;
          if (n >= 0 && n < arr.as.array.length) out = arr.as.array.items[n];
        } else if (arr.type == VALUE_PTR) {
          /* Handle new T() — baca elemen via registry v2 (memory.c). */
          bool fatal = false;
          out = memoryPtrGet(arr, idx, frame.error, &fatal);
          if (fatal) machineHalt(&frame);
        }
        if (i->result) machineSet(&frame, i->result, out);
        break;
      }
      case IR_INDEX_SET: {
        RuntimeValue arr = machineGet(&frame, i->data.index_set.array);
        RuntimeValue idx = machineGet(&frame, i->data.index_set.index);
        RuntimeValue val = machineGet(&frame, i->data.index_set.value);
        if (arr.type == VALUE_PTR) {
          /* Handle new T() — tulis elemen via registry v2 (memory.c). */
          bool fatal = false;
          memoryPtrSet(arr, idx, val, frame.error, &fatal);
          if (fatal) machineHalt(&frame);
          break;
        }
        if (arr.type == VALUE_ARRAY && idx.type == VALUE_NUMBER) {
          int n = (int)idx.as.number;
          if (n >= 0 && n < arr.as.array.length)
            arr.as.array.items[n] = val;
          else if (n == arr.as.array.length) {
            /* Auto-grow sejajar interpretMemberAssign. */
            int newLen = n + 1;
            RuntimeValue *items =
                gcrealloc(arr.as.array.items, sizeof(RuntimeValue) * (size_t)newLen);
            if (items) {
              items[newLen - 1] = val;
              arr.as.array.items = items;
              arr.as.array.length = newLen;
            }
          }
          machineSet(&frame, i->data.index_set.array, arr);
        }
        break;
      }
      case IR_ALLOC: {
        /* IR_TYPE_POINTER = new Contract (C1–C4): gccalloc(n, sizeof(T))
         * dari nama tipe di alloc.type->name; elemen type tercatat di
         * registry v3 (C2: 'T[]' direduksi ke 'T'). */
        if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_POINTER) {
          const char *tname = i->data.alloc.type->name;
          char type[256] = {0};
          if (tname) snprintf(type, sizeof(type), "%s", tname);
          size_t len = strlen(type);
          if (len >= 2 && !strcmp(type + len - 2, "[]")) type[len - 2] = '\0';
          int elemSize = 0;
          RuntimeValue out = valueNull();
          long long esz = (long long)i->data.alloc.elemSize;
          if (esz < 0) {
            /* Calloc custom, elemsize non-literal: negasi node id arg
             * dievaluasi saat eksekusi (pola IR_INTERP). */
            RuntimeValue es = valueNull();
            InterpreterResult esR = interpretNode(m->astRef, (int)-esz, frame.env, m->error);
            if (esR.flow != FLOW_NORMAL) {
              machineHalt(&frame);
              machineFree(&frame);
              return valueNull();
            }
            es = esR.value;
            if (es.type != VALUE_NUMBER || es.as.number <= 0) {
              addRuntimeError(m->error, ERR_TYPE_MISMATCH,
                              "new Contract(count, elemsize)",
                              "elemsize must be a positive number");
              machineHalt(&frame);
              machineFree(&frame);
              return valueNull();
            }
            esz = es.as.number;
          }
          long long n = 1;
          if (i->data.alloc.count) {
            RuntimeValue c = machineGet(&frame, i->data.alloc.count);
            if (c.type == VALUE_NUMBER && c.as.number > 0) n = c.as.number;
          }
          if (esz > 0) {
            /* new Contract(count, elemsize) — calloc custom: tipe
             * anotasi tidak harus dikenal, tanpa registrasi elemen
             * (sejajar jalur AST di memoryContractAssign). */
            void *handle = gccalloc((size_t)n, (size_t)esz);
            out = valuePtr(handle);
          } else if (type[0] && rupaMemorySizeOf(type, &elemSize) && elemSize > 0) {
            void *handle = gccalloc((size_t)n, (size_t)elemSize);
            if (handle) gcregsettype(handle, type);
            out = valuePtr(handle);
          }
          if (i->result) machineSet(&frame, i->result, out);
          break;
        }
        /* Buat VALUE_ARRAY / VALUE_OBJECT di register hasil.
         * count = panjang array awal; zeroed = object kosong. */
        RuntimeValue v = valueNull();
        if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_ARRAY) {
          int n = 0;
          if (i->data.alloc.count) {
            RuntimeValue c = machineGet(&frame, i->data.alloc.count);
            if (c.type == VALUE_NUMBER) n = (int)c.as.number;
          }
          v.type = VALUE_ARRAY;
          v.as.array.length = n;
          /* Buffer dari GC heap — array kosong cukup NULL (jangan
           * alloc-then-free: mismatch allocator memicu double free). */
          v.as.array.items = n > 0 ? gccalloc((size_t)n, sizeof(RuntimeValue)) : NULL;
        } else if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_OBJECT) {
          v = valueObject(NULL);
        }
        if (i->result) machineSet(&frame, i->result, v);
        break;
      }
      case IR_CALL:
        if (i->result)
          machineSet(&frame, i->result,
                     execCall(&frame, i->data.call.callee, i->data.call.args, i->data.call.count));
        else
          execCall(&frame, i->data.call.callee, i->data.call.args, i->data.call.count);
        break;
      case IR_INTERP: {
        /* Trampoline: node yang butuh runtime penuh (NODE_MOD import/
         * export/namespace) dievaluasi lewat interpreter dispatch.
         * Payload call.count = AST node id. */
        InterpreterResult r =
            interpretNode(m->astRef, (int)i->data.call.count, frame.env, m->error);
        if (i->result) machineSet(&frame, i->result, r.value);
        /* FLOW_ERROR (type check gagal) menghentikan fungsi ini. */
        if (r.flow == FLOW_ERROR) {
          machineHalt(&frame);
          machineFree(&frame);
          return valueNull();
        }
        break;
      }
      case IR_CHECK: {
        /* Semantic check struct-first: validasi value terhadap tipe
         * (scalar/struct/array-of-struct) via analyzer registry.
         * Handle VALUE_PTR: view type check via registry v3 (gcregtype)
         * — scalar check tidak berlaku.
         * Lokasi error: node value sisi kanan (presisi baris:kolom). */
        RuntimeValue v = machineGet(&frame, i->data.check.value);
        if (i->data.check.nodeId >= 0 && i->data.check.nodeId < m->astRef->length) {
          AstNode *vn = &m->astRef->ast[i->data.check.nodeId];
          setRuntimeErrorLocation(vn->line, vn->row);
        }
        if (v.type == VALUE_PTR) {
          if (!memoryHandleTypeCheck(v, i->data.check.type, m->error)) {
            machineHalt(&frame);
            machineFree(&frame);
            return valueNull();
          }
        } else if (!analyzerCheckType(i->data.check.type, v, m->error)) {
          machineHalt(&frame);
          machineFree(&frame);
          return valueNull(); /* bailing out — error sudah ditambahkan */
        }
        /* Instance new Object() lolos kontrak struct (design/object.txt):
         * stamp __type — set/update/delete strict terhadap layout.
         * Append ke tail shared object: terlihat di binding tujuan. */
        if (v.type == VALUE_OBJECT && objectIsInstance(v) && analyzerFindStruct(i->data.check.type))
          valueObjectSet(&v, "__type", valueString(i->data.check.type));
        break;
      }
      case IR_RETURN: {
        RuntimeValue ret = machineGet(&frame, i->data.return_value.value);
        /* void enforcement (sejajar interpretCall interpreter):
         * `foo(): void { return v }` dengan v non-null = TypeError fatal. */
        if (fn->return_type && fn->return_type->name && !strcmp(fn->return_type->name, "void") &&
            ret.type != VALUE_NULL) {
          if (frame.error)
            addError(frame.error, (ErrorInfo){.code = "TypeError",
                                              .message = "void function cannot return a value",
                                              .line = 0,
                                              .row = 0,
                                              .type = ERR_TYPE_MISMATCH});
          machineHalt(&frame);
        }
        machineFree(&frame);
        return ret;
      }
      case IR_JUMP:
        next = i->data.jump.target;
        break;
      case IR_BRANCH:
        next = machineTruthy(&frame, i->data.branch.condition) ? i->data.branch.then_block
                                                               : i->data.branch.else_block;
        break;
      default:
        break;
      }

      if (next) break; /* terminator mengakhiri blok */
    }

    if (!next) break; /* blok berakhir tanpa terminator */
    block = next;
  }

  machineFree(&frame);
  return valueNull();
}

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
