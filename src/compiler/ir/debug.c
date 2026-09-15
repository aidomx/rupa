#include <rupa.h>

/* ============================================================
 * debug.c — IR printer (untuk --test-ir)
 *
 * Menampilkan struktur IRModule: function, block, dan tiap
 * instruksi dalam bentuk teks ringkas.
 * ============================================================ */

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
  case IR_INTERP: return "interp";
  case IR_RETURN: return "ret";
  case IR_JUMP: return "jump";
  case IR_BRANCH: return "branch";
  case IR_ALLOC: return "alloc";
  case IR_REALLOC: return "realloc";
  case IR_FREE: return "free";
  case IR_CAST: return "cast";
  default: return "?";
  }
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
    case IR_CONST_STRING: printf("\"%s\"", v->data.constant.as.string ? v->data.constant.as.string : ""); break;
    case IR_CONST_NULL: printf("null"); break;
    default: printf("const?"); break;
    }
    break;
  case IR_VALUE_TEMP: printf("t%u", v->id); break;
  default:
    if (v->data.name) printf("%s", v->data.name);
    else printf("v%u", v->id);
    break;
  }
}

static void irPrintInstruction(IRInstruction *i) {
  printf("    %-12s ", irOpName(i->op));

  switch (i->op) {
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
    break;
  case IR_NEG:
  case IR_NOT:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.unary.value);
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
  case IR_AND:
  case IR_OR:
    irPrintValue(i->result);
    printf(" <- ");
    irPrintValue(i->data.binary.left);
    printf(", ");
    irPrintValue(i->data.binary.right);
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
    printf(")");
    break;
  case IR_RETURN:
    printf("ret ");
    irPrintValue(i->data.return_value.value);
    break;
  case IR_INTERP:
    if (i->result) {
      irPrintValue(i->result);
      printf(" <- ");
    }
    printf("node#%zu", i->data.call.count);
    break;
  case IR_JUMP:
    printf("-> %s", i->data.jump.target && i->data.jump.target->name
                        ? i->data.jump.target->name : "?");
    break;
  case IR_BRANCH:
    printf("if ");
    irPrintValue(i->data.branch.condition);
    printf(" -> %s else %s",
           i->data.branch.then_block && i->data.branch.then_block->name
               ? i->data.branch.then_block->name : "?",
           i->data.branch.else_block && i->data.branch.else_block->name
               ? i->data.branch.else_block->name : "?");
    break;
  case IR_ALLOC:
    irPrintValue(i->result);
    printf(" <- alloc(%s%s)",
           i->data.alloc.type && i->data.alloc.type->name ? i->data.alloc.type->name : "?",
           i->data.alloc.zeroed ? ", zeroed" : "");
    break;
  case IR_FREE:
    printf("free ");
    irPrintValue(i->data.free.pointer);
    break;
  case IR_CAST:
    irPrintValue(i->result);
    printf(" <- cast ");
    irPrintValue(i->data.cast.value);
    break;
  default:
    break;
  }

  printf("\n");
}

void debugIRModule(IRModule *ir) {
  if (!ir) {
    printf("<ir: null>\n");
    return;
  }

  printf("> IR Structure\n");
  for (IRFunction *f = ir->first_function; f; f = f->next) {
    printf("function %s(", f->name ? f->name : "?");
    for (size_t i = 0; i < f->param_count; i++) {
      if (i) printf(", ");
      irPrintValue(f->params[i]);
    }
    printf("):\n");

    for (IRBlock *blk = f->first_block; blk; blk = blk->next) {
      printf("  %s:\n", blk->name ? blk->name : "?");
      for (IRInstruction *i = blk->first; i; i = i->next)
        irPrintInstruction(i);
    }
  }
}
