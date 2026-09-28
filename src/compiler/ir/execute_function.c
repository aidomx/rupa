#include <rupa.h>
#include "execute_internal.h"

/* execute_function.c — eksekusi satu IRFunction: frame env baru, param
 * di-bind by name, loop blok mengikuti terminator, handler semua
 * instruksi IR. Unit hasil split execute.c:
 *   execute_machine.c   — core mesin (register temp, get/set)
 *   execute_call.c      — execCall + lookup fungsi
 *   execute.c           — entry point modul
 */

/* Eksekusi satu IRFunction: frame env baru, param di-bind by name,
 * lalu jalan blok demi blok mengikuti terminator. */
RuntimeValue execFunction(IRMachine *m, IRFunction *fn, RuntimeValue *args, int argc) {
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
    /* Error fatal menghentikan eksekusi: cek per-blok (bukan
     * per-instruksi). Cabang halt di handler tetap break keluar; satu
     * instruksi sisa setelah fatal tidak mengubah hasil (nilai sudah
     * error dan mesin langsung bailing out). */
    if (machineHalted(&frame)) break;
    IRBlock *next = NULL;

    for (IRInstruction *i = block->first; i; i = i->next) {
      /* Halt fatal (TypeError/MemoryError/ConstError dari call/sebelumnya)
       * menghentikan sisa instruksi blok — tanpa ini nilai hasil call yang
       * gagal kontrak tetap mengalir ke instruksi berikutnya. */
      if (machineHalted(&frame)) break;
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
      case IR_LT:
        /* Fast path integer (loop panas): dua operand number dihitung
         * langsung di domain integer — tanpa konversi double + floor.
         * Jalur lambat (decimal/campuran/concat) jatuh ke evalBinary. */
        if (i->result) {
          RuntimeValue l = machineGet(&frame, i->data.binary.left);
          RuntimeValue r = machineGet(&frame, i->data.binary.right);
          if (l.type == VALUE_NUMBER && r.type == VALUE_NUMBER)
            machineSet(&frame, i->result,
                       i->op == IR_ADD ? valueNumber(l.as.number + r.as.number)
                                       : valueBoolean(l.as.number < r.as.number));
          else
            machineSet(&frame, i->result, evalBinaryValue(i->op, l, r));
        }
        break;
      case IR_SUB:
      case IR_MUL:
      case IR_DIV:
      case IR_MOD:
      case IR_EQ:
      case IR_NE:
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
            /* Member private (metadata `_private`, design namespace/fn):
             * nilai null di surface + nama tercatat di _private → akses
             * luar = PrivateError, bukan null senyap. Selaras interpreter
             * (interpretMember). */
            if (out.type == VALUE_NULL && key && frame.error) {
              RuntimeValue priv;
              if (valueObjectGet(obj, "_private", &priv) && priv.type == VALUE_OBJECT) {
                RuntimeValue found;
                if (valueObjectGet(priv, key, &found)) {
                  static char msgPriv[256];
                  snprintf(msgPriv, sizeof(msgPriv),
                           "'%s' is private and cannot be accessed from outside", key);
                  addError(frame.error, (ErrorInfo){.code = "PrivateError",
                                                    .message = msgPriv,
                                                    .line = 0,
                                                    .row = 0,
                                                    .type = ERR_UNDEFINED_VAR});
                  machineHalt(&frame);
                  machineFree(&frame);
                  return valueNull();
                }
              }
            }
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
      case IR_PROBE_ABSENT: {
        /* "Nama BELUM ter-bind di env?" — probe sebelum loop untuk
         * menentukan apakah binding loop implisit (design loop). */
        IRValue *t = i->data.probe.target;
        bool absent = true;
        if (t && t->data.name) {
          RuntimeValue probe;
          absent = !semGet(frame.env, t->data.name, &probe);
        }
        if (i->result)
          machineSet(&frame, i->result, valueBoolean(absent));
        break;
      }
      case IR_UNBIND: {
        /* Lepas binding variabel (design loop): binding loop implisit
         * di-unbind setelah loop — pembacaan nama kembali undefined.
         * Bersyarat bila condition ada (hasil probe awal loop). */
        IRValue *t = i->data.unbind.target;
        bool doUnbind = true;
        if (i->data.unbind.condition)
          doUnbind = machineTruthy(&frame, i->data.unbind.condition);
        if (doUnbind && t && t->data.name)
          semUnsetLocal(frame.env, t->data.name);
        break;
      }
      case IR_MARK_PUB: {
        /* Tandai binding `pub` (design fn/namespace): export surface. */
        IRValue *t = i->data.mark_pub.target;
        if (t && t->data.name)
          semMarkPub(frame.env, t->data.name);
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
              addRuntimeError(m->error, ERR_TYPE_MISMATCH, "new Contract(count, elemsize)",
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
        if (i->data.check.funcName) {
          /* Return-type contract: pesan error presisi dengan nama fungsi
           * (analyzerCheckReturnType). */
          if (!analyzerCheckReturnType(i->data.check.funcName, i->data.check.type, v, m->error)) {
            machineHalt(&frame);
            machineFree(&frame);
            return valueNull();
          }
        } else if (v.type == VALUE_PTR) {
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
          if (frame.error) {
            static char message[256];
            snprintf(message, sizeof(message), "function '%s' is void and cannot return a value",
                     fn->name ? fn->name : "?");
            addError(frame.error, (ErrorInfo){.code = "TypeError",
                                              .message = message,
                                              .line = 0,
                                              .row = 0,
                                              .type = ERR_TYPE_MISMATCH});
          }
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
