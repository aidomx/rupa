#include <rupa.h>

/* ============================================================
 * debug.c — Opcode disassembler (untuk `rupa ir` / --test-ir)
 *
 * Menampilkan IRModule sebagai listing opcode: signature fungsi
 * (param + return type), block, dan tiap instruksi dengan SEMUA
 * operand — nomor instruksi (pc) memudahkan membaca jump/branch.
 * Semua opcode enum IROpcode tercover eksplisit; instruksi yang
 * belum ditangani jatuh ke marker `<op ?>` alih-alih dicetak
 * diam-diam tanpa operand.
 * ============================================================ */

/* --- Nama opcode: tabel lengkap atas enum IROpcode --- */

static const char *irOpName(IROpcode op) {
  switch (op) {
  case IR_NOP: return "nop";
  case IR_CONST: return "const";
  case IR_LOAD: return "load";
  case IR_STORE: return "store";
  case IR_ADD: return "add";
  case IR_SUB: return "sub";
  case IR_MUL: return "mul";
  case IR_DIV: return "div";
  case IR_MOD: return "mod";
  case IR_NEG: return "neg";
  case IR_EQ: return "eq";
  case IR_NE: return "ne";
  case IR_LT: return "lt";
  case IR_LE: return "le";
  case IR_GT: return "gt";
  case IR_GE: return "ge";
  case IR_NOT: return "not";
  case IR_AND: return "and";
  case IR_OR: return "or";
  case IR_MEMBER_GET: return "member_get";
  case IR_MEMBER_SET: return "member_set";
  case IR_INDEX_GET: return "index_get";
  case IR_INDEX_SET: return "index_set";
  case IR_CALL: return "call";
  case IR_RETURN: return "ret";
  case IR_INTERP: return "interp";
  case IR_CHECK: return "check";
  case IR_JUMP: return "jump";
  case IR_BRANCH: return "branch";
  case IR_ALLOC: return "alloc";
  case IR_REALLOC: return "realloc";
  case IR_FREE: return "free";
  case IR_STRSLOT_GET: return "strslot_get";
  case IR_STRSLOT_SET: return "strslot_set";
  case IR_PROBE_ABSENT: return "probe_absent";
  case IR_UNBIND: return "unbind";
  case IR_MARK_PUB: return "mark_pub";
  case IR_CAST: return "cast";
  default: return "?";
  }
}

/* --- Value operand --- */

/* String ter-escape (baris baru, quote, byte non-printable) agar
 * konstanta string multi-baris tidak merusak format listing. */
static void irPrintEscaped(const char *s) {
  fputc('"', stdout);
  if (s) {
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
      switch (*p) {
      case '\\': fputs("\\\\", stdout); break;
      case '"': fputs("\\\"", stdout); break;
      case '\n': fputs("\\n", stdout); break;
      case '\r': fputs("\\r", stdout); break;
      case '\t': fputs("\\t", stdout); break;
      default:
        if (*p < 32 || *p >= 127) printf("\\%03o", *p);
        else fputc(*p, stdout);
      }
    }
  }
  fputc('"', stdout);
}

static void irPrintValue(IRValue *v) {
  if (!v) {
    printf("_");
    return;
  }
  switch (v->kind) {
  case IR_VALUE_CONSTANT:
    switch (v->data.constant.kind) {
    case IR_CONST_NUMBER: printf("%lld", (long long)v->data.constant.as.number); break;
    case IR_CONST_DECIMAL: printf("%g", v->data.constant.as.decimal); break;
    case IR_CONST_BOOLEAN: printf("%s", v->data.constant.as.boolean ? "true" : "false"); break;
    case IR_CONST_STRING: irPrintEscaped(v->data.constant.as.string); break;
    case IR_CONST_NULL: printf("null"); break;
    default: printf("const?"); break;
    }
    break;
  case IR_VALUE_TEMP: printf("t%u", v->id); break;
  case IR_VALUE_FUNCTION:
    /* Slot fungsi — bedakan dari variable biasa agar call terbaca. */
    printf("fn:%s", v->data.name ? v->data.name : "?");
    break;
  default:
    if (v->data.name) printf("%s", v->data.name);
    else printf("v%u", v->id);
    break;
  }
}

/* Nama tipe IR (recursive untuk IR_TYPE_ARRAY: number[][], dst). */
static void irTypeName(IRType *t, char *out, size_t cap) {
  if (!out || cap == 0) return;
  out[0] = '\0';
  if (!t) {
    snprintf(out, cap, "?");
    return;
  }
  if (t->kind == IR_TYPE_ARRAY && t->element) {
    char elem[128];
    irTypeName(t->element, elem, sizeof(elem));
    snprintf(out, cap, "%s[]", elem);
    return;
  }
  snprintf(out, cap, "%s", t->name ? t->name : "?");
}

static const char *irBlockName(IRBlock *b) {
  return b ? (b->name ? b->name : "?") : "?";
}

/* --- Instruksi: satu baris per opcode, SEMUA operand --- */

static void irDisasmInstruction(IRInstruction *i, int pc) {
  printf("  %04d  %-12s ", pc, irOpName(i->op));

  switch (i->op) {
  /* dst <- src */
  case IR_CONST:
  case IR_LOAD:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.unary.value);
    break;

  case IR_STORE:
    irPrintValue(i->data.store.target);
    printf(" <- ");
    irPrintValue(i->data.store.value);
    if (i->data.store.isConst) printf("  ; const lock");
    break;

  case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
  case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
  case IR_AND: case IR_OR:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.binary.left);
    printf(", ");
    irPrintValue(i->data.binary.right);
    break;

  case IR_NEG:
  case IR_NOT:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.unary.value);
    break;

  case IR_MEMBER_GET:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.member_get.object);
    printf(".%s", i->data.member_get.member ? i->data.member_get.member : "?");
    break;

  case IR_MEMBER_SET:
    irPrintValue(i->data.member_set.object);
    printf(".%s <- ", i->data.member_set.member ? i->data.member_set.member : "?");
    irPrintValue(i->data.member_set.value);
    break;

  case IR_INDEX_GET:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.index_get.array);
    printf("[");
    irPrintValue(i->data.index_get.index);
    printf("]");
    break;

  case IR_INDEX_SET:
    irPrintValue(i->data.index_set.array);
    printf("[");
    irPrintValue(i->data.index_set.index);
    printf("] <- ");
    irPrintValue(i->data.index_set.value);
    break;

  case IR_CALL:
    if (i->result) {
      irPrintValue(i->result);
      printf(" <- ");
    }
    irPrintValue(i->data.call.callee);
    printf("(");
    for (size_t k = 0; k < i->data.call.count; k++) {
      if (k) printf(", ");
      irPrintValue(i->data.call.args[k]);
    }
    printf(")  ; argc=%zu", i->data.call.count);
    break;

  case IR_RETURN:
    irPrintValue(i->data.return_value.value);
    break;

  case IR_INTERP:
    if (i->result) {
      irPrintValue(i->result);
      printf(" <- ");
    }
    printf("node#%zu", i->data.call.count);
    break;

  case IR_CHECK:
    irPrintValue(i->data.check.value);
    printf(" : %s", i->data.check.type ? i->data.check.type : "?");
    if (i->data.check.funcName)
      printf("  ; fn=%s", i->data.check.funcName);
    if (i->data.check.nodeId >= 0)
      printf(" node#%d", i->data.check.nodeId);
    break;

  case IR_PROBE_ABSENT:
    irPrintValue(i->result);
    printf(" <- absent(");
    irPrintValue(i->data.probe.target);
    printf(")");
    break;

  case IR_UNBIND:
    printf("unbind ");
    irPrintValue(i->data.unbind.target);
    if (i->data.unbind.condition) {
      printf(" if ");
      irPrintValue(i->data.unbind.condition);
    }
    break;

  case IR_MARK_PUB:
    printf("mark_pub ");
    irPrintValue(i->data.mark_pub.target);
    break;

  case IR_JUMP:
    printf("-> %s", irBlockName(i->data.jump.target));
    break;

  case IR_BRANCH:
    printf("if ");
    irPrintValue(i->data.branch.condition);
    printf(" -> %s else %s", irBlockName(i->data.branch.then_block),
           irBlockName(i->data.branch.else_block));
    break;

  case IR_ALLOC:
    irPrintValue(i->result);
    printf(" <- alloc(");
    if (i->data.alloc.type) {
      char tn[128];
      irTypeName(i->data.alloc.type, tn, sizeof(tn));
      printf("%s", tn);
    } else {
      printf("?");
    }
    if (i->data.alloc.count) {
      printf(", count=");
      irPrintValue(i->data.alloc.count);
    }
    if (i->data.alloc.elemSize > 0)
      printf(", elemsz=%d", i->data.alloc.elemSize);
    if (i->data.alloc.zeroed)
      printf(", zeroed");
    printf(")");
    break;

  case IR_REALLOC:
    irPrintValue(i->result);
    printf(" <- realloc(");
    irPrintValue(i->data.realloc.pointer);
    printf(", ");
    irPrintValue(i->data.realloc.size);
    printf(")");
    break;

  case IR_FREE:
    irPrintValue(i->data.free.pointer);
    break;

  case IR_STRSLOT_GET:
    irPrintValue(i->result);
    printf(" <- strslot(");
    irPrintValue(i->data.strslot_get.pointer);
    printf(")");
    break;

  case IR_STRSLOT_SET:
    printf("strslot(");
    irPrintValue(i->data.strslot_set.pointer);
    printf(") <- ");
    irPrintValue(i->data.strslot_set.value);
    break;

  case IR_CAST:
    irPrintValue(i->result);
    printf(" <- cast ");
    irPrintValue(i->data.cast.value);
    break;

  case IR_NOP:
    break;

  default:
    /* Opcode enum baru yang belum didisasm — jangan diam. */
    printf("<op %d ?>", (int)i->op);
    break;
  }

  printf("\n");
}

/* --- Fungsi & modul --- */

static int irDisasmFunction(IRFunction *f, int pcBase) {
  int pc = pcBase;

  printf("; ---- function %s(", f->name ? f->name : "?");
  for (size_t i = 0; i < f->param_count; i++) {
    if (i) printf(", ");
    irPrintValue(f->params[i]);
  }
  printf(")");
  if (f->return_type) {
    char tn[128];
    irTypeName(f->return_type, tn, sizeof(tn));
    printf(" -> %s", tn);
  }
  printf("\n");

  for (IRBlock *blk = f->first_block; blk; blk = blk->next) {
    /* Hitung instruksi block untuk header. */
    int n = 0;
    for (IRInstruction *c = blk->first; c; c = c->next) n++;
    printf("%s:  ; %d insns\n", blk->name ? blk->name : "?", n);
    for (IRInstruction *i = blk->first; i; i = i->next)
      irDisasmInstruction(i, pc++);
  }

  return pc;
}

void debugIRModule(IRModule *ir) {
  if (!ir) {
    printf("<ir: null>\n");
    return;
  }

  /* Ringkasan modul: fungsi, block, instruksi + histogram opcode. */
  int totalFn = 0, totalBlocks = 0, totalInsns = 0;
  int histogram[IR_CAST + 1] = {0};

  for (IRFunction *f = ir->first_function; f; f = f->next) {
    totalFn++;
    for (IRBlock *blk = f->first_block; blk; blk = blk->next) {
      totalBlocks++;
      for (IRInstruction *i = blk->first; i; i = i->next) {
        totalInsns++;
        if (i->op >= 0 && i->op <= IR_CAST) histogram[i->op]++;
      }
    }
  }

  printf("> IR Opcode Listing\n");
  printf("; module: %d function(s), %d block(s), %d insn(s)\n", totalFn, totalBlocks,
         totalInsns);
  if (totalInsns > 0) {
    printf("; opcodes:");
    for (int op = 0; op <= IR_CAST; op++) {
      if (!histogram[op]) continue;
      printf(" %s x%d", irOpName((IROpcode)op), histogram[op]);
    }
    printf("\n");
  }

  int pc = 0;
  for (IRFunction *f = ir->first_function; f; f = f->next) {
    printf("\n");
    pc = irDisasmFunction(f, pc);
  }
}
