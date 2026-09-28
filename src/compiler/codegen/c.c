#include "rupa.h"

#include <errno.h>
#include <stdarg.h>

/* readlink (/proc/self/exe) untuk resolve root project saat compile -o
 * dari direktori mana pun — preseden serve.c (guard platform). */
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#define CODEGEN_HAS_READLINK 1
#else
#define CODEGEN_HAS_READLINK 0
#endif

/* Native C backend for the current IR.  The generated program uses the
 * existing Rupa runtime, but the hot control/data flow is emitted as C
 * instead of going through executeIR(). */

typedef struct {
  const IRValue **items;
  size_t count;
  size_t cap;
} ValueList;

typedef struct {
  const IRFunction **items;
  size_t count;
  size_t cap;
} FunctionList;

typedef struct {
  FILE *fp;
  ValueList temps;
  FunctionList functions;
  bool unsupported;
  char reason[256];
} CGen;

static void fail(CGen *g, const char *fmt, ...) {
  if (g->unsupported) return;
  g->unsupported = true;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(g->reason, sizeof(g->reason), fmt, ap);
  va_end(ap);
}

static int pushValue(ValueList *list, const IRValue *v) {
  if (!v || v->kind != IR_VALUE_TEMP) return 1;
  for (size_t i = 0; i < list->count; i++)
    if (list->items[i] == v) return 1;
  if (list->count == list->cap) {
    size_t cap = list->cap ? list->cap * 2 : 32;
    const IRValue **next = realloc(list->items, cap * sizeof(*next));
    if (!next) return 0;
    list->items = next;
    list->cap = cap;
  }
  list->items[list->count++] = v;
  return 1;
}

static int pushFunction(FunctionList *list, const IRFunction *fn) {
  if (!fn) return 1;
  if (list->count == list->cap) {
    size_t cap = list->cap ? list->cap * 2 : 16;
    const IRFunction **next = realloc(list->items, cap * sizeof(*next));
    if (!next) return 0;
    list->items = next;
    list->cap = cap;
  }
  list->items[list->count++] = fn;
  return 1;
}

static void collectValue(CGen *g, IRValue *v) {
  if (!v) return;
  if (!pushValue(&g->temps, v)) fail(g, "out of memory while collecting IR values");
}

static void collectInstruction(CGen *g, IRInstruction *i) {
  if (!i) return;
  collectValue(g, i->result);
  switch (i->op) {
  case IR_CONST: collectValue(g, i->data.unary.value); break;
  case IR_LOAD: collectValue(g, i->data.unary.value); break;
  case IR_STORE: collectValue(g, i->data.store.target); collectValue(g, i->data.store.value); break;
  case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
  case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
  case IR_AND: case IR_OR:
    collectValue(g, i->data.binary.left); collectValue(g, i->data.binary.right); break;
  case IR_NEG: case IR_NOT: collectValue(g, i->data.unary.value); break;
  case IR_MEMBER_GET:
    collectValue(g, i->data.member_get.object); break;
  case IR_MEMBER_SET:
    collectValue(g, i->data.member_set.object); collectValue(g, i->data.member_set.value); break;
  case IR_INDEX_GET:
    collectValue(g, i->data.index_get.array); collectValue(g, i->data.index_get.index); break;
  case IR_INDEX_SET:
    collectValue(g, i->data.index_set.array); collectValue(g, i->data.index_set.index); collectValue(g, i->data.index_set.value); break;
  case IR_CALL:
    collectValue(g, i->data.call.callee);
    for (size_t n = 0; n < i->data.call.count; n++) collectValue(g, i->data.call.args[n]);
    break;
  case IR_RETURN: collectValue(g, i->data.return_value.value); break;
  case IR_BRANCH: collectValue(g, i->data.branch.condition); break;
  case IR_ALLOC: collectValue(g, i->data.alloc.count); break;
  case IR_REALLOC: collectValue(g, i->data.realloc.pointer); collectValue(g, i->data.realloc.size); break;
  case IR_FREE: collectValue(g, i->data.free.pointer); break;
  case IR_STRSLOT_GET: collectValue(g, i->data.strslot_get.pointer); break;
  case IR_STRSLOT_SET: collectValue(g, i->data.strslot_set.pointer); collectValue(g, i->data.strslot_set.value); break;
  case IR_CAST: collectValue(g, i->data.cast.value); break;
  case IR_CHECK: collectValue(g, i->data.check.value); break;
  case IR_PROBE_ABSENT: collectValue(g, i->data.probe.target); break;
  case IR_UNBIND:
    collectValue(g, i->data.unbind.target); collectValue(g, i->data.unbind.condition); break;
  case IR_MARK_PUB: collectValue(g, i->data.mark_pub.target); break;
  default: break;
  }
}

static void collectModule(CGen *g, IRModule *ir) {
  for (IRFunction *fn = ir->first_function; fn; fn = fn->next) {
    if (!pushFunction(&g->functions, fn)) {
      fail(g, "out of memory while collecting IR functions");
      return;
    }
    for (size_t p = 0; p < fn->param_count; p++) collectValue(g, fn->params[p]);
    for (IRBlock *b = fn->first_block; b; b = b->next)
      for (IRInstruction *i = b->first; i; i = i->next) collectInstruction(g, i);
  }
}

static void cstr(FILE *fp, const char *s) {
  fputc('"', fp);
  if (s) {
    for (; *s; s++) {
      unsigned char c = (unsigned char)*s;
      switch (c) {
      case '\\': fputs("\\\\", fp); break;
      case '"': fputs("\\\"", fp); break;
      case '\n': fputs("\\n", fp); break;
      case '\r': fputs("\\r", fp); break;
      case '\t': fputs("\\t", fp); break;
      default:
        if (c < 32 || c >= 127) fprintf(fp, "\\%03o", c);
        else fputc(c, fp);
      }
    }
  }
  fputc('"', fp);
}

static void ident(FILE *fp, const char *name) {
  if (!name || !*name) { fputs("_", fp); return; }
  if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) fputc('_', fp);
  for (const unsigned char *p = (const unsigned char *)name; *p; p++)
    fputc(isalnum(*p) || *p == '_' ? *p : '_', fp);
}

static const char *opString(IROpcode op) {
  switch (op) {
  case IR_ADD: return "+"; case IR_SUB: return "-"; case IR_MUL: return "*";
  case IR_DIV: return "/"; case IR_MOD: return "%";
  default: return NULL;
  }
}

static const char *tempName(const IRValue *v) {
  static char buf[64];
  snprintf(buf, sizeof(buf), "v%u", v ? v->id : 0);
  return buf;
}

static const IRFunction *findGeneratedFunction(const CGen *g, const IRValue *v) {
  if (!v || !v->data.name) return NULL;
  for (size_t i = 0; i < g->functions.count; i++)
    if (g->functions.items[i]->name && !strcmp(g->functions.items[i]->name, v->data.name))
      return g->functions.items[i];
  return NULL;
}

static void emitValue(CGen *g, RuntimeValue *dummy, const IRValue *v, const char *env) {
  (void)dummy;
  FILE *fp = g->fp;
  if (!v) { fputs("valueNull()", fp); return; }
  switch (v->kind) {
  case IR_VALUE_TEMP: fprintf(fp, "%s", tempName(v)); break;
  case IR_VALUE_CONSTANT:
    switch (v->data.constant.kind) {
    case IR_CONST_NUMBER: fprintf(fp, "valueNumber(%lldLL)", (long long)v->data.constant.as.number); break;
    case IR_CONST_DECIMAL: fprintf(fp, "valueDecimal(%.17g)", v->data.constant.as.decimal); break;
    case IR_CONST_BOOLEAN: fprintf(fp, "valueBoolean(%s)", v->data.constant.as.boolean ? "true" : "false"); break;
    case IR_CONST_STRING: fputs("valueString(", fp); cstr(fp, v->data.constant.as.string); fputc(')', fp); break;
    case IR_CONST_NULL: default: fputs("valueNull()", fp); break;
    }
    break;
  case IR_VALUE_FUNCTION:
  case IR_VALUE_PARAM:
  case IR_VALUE_LOCAL:
  case IR_VALUE_GLOBAL:
    fprintf(fp, "rupa_get(%s, ", env); cstr(fp, v->data.name); fputs(")", fp); break;
  default: fputs("valueNull()", fp); break;
  }
}

static void emitTempAssignPrefix(CGen *g, IRValue *result) {
  if (result && result->kind == IR_VALUE_TEMP) fprintf(g->fp, "  %s = ", tempName(result));
}

static void emitCallArgs(CGen *g, IRValue **args, size_t count, const char *env) {
  FILE *fp = g->fp;
  fprintf(fp, "(RuntimeValue[]){");
  for (size_t i = 0; i < count; i++) {
    if (i) fputs(", ", fp);
    emitValue(g, NULL, args[i], env);
  }
  fputs("}", fp);
}

static void emitInstruction(CGen *g, IRInstruction *i, const char *env, const char *err) {
  FILE *fp = g->fp;
  if (!i) return;
  switch (i->op) {
  case IR_NOP: break;
  case IR_CONST:
    emitTempAssignPrefix(g, i->result);
    emitValue(g, NULL, i->data.unary.value, env);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_LOAD:
    emitTempAssignPrefix(g, i->result);
    emitValue(g, NULL, i->data.unary.value, env);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_STORE:
    if (i->data.store.target && i->data.store.target->kind == IR_VALUE_TEMP) {
      fprintf(fp, "  %s = ", tempName(i->data.store.target));
      emitValue(g, NULL, i->data.store.value, env);
      fputs(";\n", fp);
    } else if (i->data.store.target && i->data.store.target->data.name) {
      fprintf(fp, "  rupa_set(%s, ", env);
      cstr(fp, i->data.store.target->data.name);
      fputs(", ", fp);
      emitValue(g, NULL, i->data.store.value, env);
      fprintf(fp, ", %s);\n", i->data.store.isConst ? "true" : "false");
    }
    break;
  case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD: {
    const char *op = opString(i->op);
    if (!op) { fail(g, "unsupported arithmetic opcode"); return; }
    emitTempAssignPrefix(g, i->result);
    fputs("rupa_binary(", fp); cstr(fp, op); fputs(", ", fp);
    emitValue(g, NULL, i->data.binary.left, env); fputs(", ", fp);
    emitValue(g, NULL, i->data.binary.right, env); fputs(", ", fp); fputs(err, fp); fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  }
  case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE: {
    const char *op = i->op == IR_EQ ? "==" : i->op == IR_NE ? "!=" : i->op == IR_LT ? "<" :
                     i->op == IR_LE ? "<=" : i->op == IR_GT ? ">" : ">=";
    emitTempAssignPrefix(g, i->result);
    fprintf(fp, "valueCompare("); cstr(fp, op); fputs(", ", fp);
    emitValue(g, NULL, i->data.binary.left, env); fputs(", ", fp);
    emitValue(g, NULL, i->data.binary.right, env); fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  }
  case IR_AND: case IR_OR:
    emitTempAssignPrefix(g, i->result);
    fprintf(fp, "valueBoolean(valueTruthy(");
    emitValue(g, NULL, i->data.binary.left, env);
    fputs(i->op == IR_AND ? ") && valueTruthy(" : ") || valueTruthy(", fp);
    emitValue(g, NULL, i->data.binary.right, env);
    fputs("))", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_NEG:
    emitTempAssignPrefix(g, i->result);
    fprintf(fp, "rupa_neg("); emitValue(g, NULL, i->data.unary.value, env); fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_NOT:
    emitTempAssignPrefix(g, i->result);
    fprintf(fp, "valueBoolean(!valueTruthy("); emitValue(g, NULL, i->data.unary.value, env); fputs("))", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_MEMBER_GET:
    emitTempAssignPrefix(g, i->result);
    fputs("rupa_member_get(", fp); emitValue(g, NULL, i->data.member_get.object, env); fputs(", ", fp);
    cstr(fp, i->data.member_get.member); fputs(", ", fp); fputs(err, fp); fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_MEMBER_SET:
    if (i->data.member_set.object && i->data.member_set.object->kind == IR_VALUE_TEMP) {
      fprintf(fp, "  %s = rupa_member_set(", tempName(i->data.member_set.object));
      emitValue(g, NULL, i->data.member_set.object, env); fputs(", ", fp);
      cstr(fp, i->data.member_set.member); fputs(", ", fp);
      emitValue(g, NULL, i->data.member_set.value, env); fprintf(fp, ", %s);\n", err);
    } else if (i->data.member_set.object && i->data.member_set.object->data.name) {
      fprintf(fp, "  rupa_set(%s, ", env); cstr(fp, i->data.member_set.object->data.name); fputs(", rupa_member_set(", fp);
      emitValue(g, NULL, i->data.member_set.object, env); fputs(", ", fp);
      cstr(fp, i->data.member_set.member); fputs(", ", fp);
      emitValue(g, NULL, i->data.member_set.value, env); fprintf(fp, ", %s), false);\n", err);
    }
    break;
  case IR_INDEX_GET:
    emitTempAssignPrefix(g, i->result);
    fputs("rupa_index_get(", fp); emitValue(g, NULL, i->data.index_get.array, env); fputs(", ", fp);
    emitValue(g, NULL, i->data.index_get.index, env); fprintf(fp, ", %s)", err);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_INDEX_SET:
    if (i->data.index_set.array && i->data.index_set.array->kind == IR_VALUE_TEMP) {
      fprintf(fp, "  %s = rupa_index_set(", tempName(i->data.index_set.array));
      emitValue(g, NULL, i->data.index_set.array, env); fputs(", ", fp);
      emitValue(g, NULL, i->data.index_set.index, env); fputs(", ", fp);
      emitValue(g, NULL, i->data.index_set.value, env); fprintf(fp, ", %s);\n", err);
    } else if (i->data.index_set.array && i->data.index_set.array->data.name) {
      fprintf(fp, "  rupa_set(%s, ", env); cstr(fp, i->data.index_set.array->data.name); fputs(", rupa_index_set(", fp);
      emitValue(g, NULL, i->data.index_set.array, env); fputs(", ", fp);
      emitValue(g, NULL, i->data.index_set.index, env); fputs(", ", fp);
      emitValue(g, NULL, i->data.index_set.value, env); fprintf(fp, ", %s), false);\n", err);
    }
    break;
  case IR_CALL: {
    const IRValue *callee = i->data.call.callee;
    if (findGeneratedFunction(g, callee)) {
      fprintf(fp, "  ");
      if (i->result && i->result->kind == IR_VALUE_TEMP) fprintf(fp, "%s = ", tempName(i->result));
      fprintf(fp, "rupa_fn_"); ident(fp, callee->data.name);
      fprintf(fp, "(%s, %s, %zu, ", env, err, i->data.call.count);
      emitCallArgs(g, i->data.call.args, i->data.call.count, env);
      fputs(");\n", fp);
    } else {
      if (i->result && i->result->kind == IR_VALUE_TEMP) fprintf(fp, "  %s = ", tempName(i->result));
      else fputs("  ", fp);
      if (callee && callee->data.name) {
        fprintf(fp, "rupa_call_named("); cstr(fp, callee->data.name); fprintf(fp, ", %s, %s, %zu, ", env, err, i->data.call.count);
      } else {
        fprintf(fp, "rupa_call("); emitValue(g, NULL, callee, env); fprintf(fp, ", %s, %s, %zu, ", env, err, i->data.call.count);
      }
      emitCallArgs(g, i->data.call.args, i->data.call.count, env); fputs(");\n", fp);
    }
    break;
  }
  case IR_RETURN:
    fputs("  return ", fp); emitValue(g, NULL, i->data.return_value.value, env); fputs(";\n", fp); break;
  case IR_JUMP:
    fprintf(fp, "  goto block_%u;\n", i->data.jump.target ? i->data.jump.target->id : 0); break;
  case IR_BRANCH:
    fputs("  if (valueTruthy(", fp); emitValue(g, NULL, i->data.branch.condition, env);
    fprintf(fp, ")) goto block_%u; else goto block_%u;\n",
            i->data.branch.then_block ? i->data.branch.then_block->id : 0,
            i->data.branch.else_block ? i->data.branch.else_block->id : 0);
    break;
  case IR_ALLOC:
    emitTempAssignPrefix(g, i->result);
    if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_ARRAY) {
      fputs("rupa_alloc_array(", fp); emitValue(g, NULL, i->data.alloc.count, env); fputs(")", fp);
    } else if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_OBJECT) {
      fputs("valueObject(NULL)", fp);
    } else if (i->data.alloc.type && i->data.alloc.type->kind == IR_TYPE_POINTER) {
      fputs("rupa_alloc_ptr(", fp); emitValue(g, NULL, i->data.alloc.count, env);
      fprintf(fp, ", %d, ", i->data.alloc.elemSize); cstr(fp, i->data.alloc.type->name); fputs(")", fp);
    } else {
      fail(g, "unsupported IR_ALLOC type"); return;
    }
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_REALLOC:
    emitTempAssignPrefix(g, i->result);
    fputs("rupa_realloc(", fp); emitValue(g, NULL, i->data.realloc.pointer, env); fputs(", ", fp);
    emitValue(g, NULL, i->data.realloc.size, env); fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_FREE:
    fputs("  rupa_free(", fp); emitValue(g, NULL, i->data.free.pointer, env); fputs(");\n", fp); break;
  case IR_CHECK:
    fputs("  rupa_check(", fp); emitValue(g, NULL, i->data.check.value, env); fputs(", ", fp);
    cstr(fp, i->data.check.type); fputs(", ", fp); cstr(fp, i->data.check.funcName); fprintf(fp, ", %s);\n", err); break;
  case IR_PROBE_ABSENT:
    /* Binding loop implisit (design loop): result <- "nama belum ada?". */
    emitTempAssignPrefix(g, i->result);
    fputs("rupa_absent(env, ", fp);
    cstr(fp, i->data.probe.target ? i->data.probe.target->data.name : "");
    fputs(")", fp);
    if (i->result && i->result->kind == IR_VALUE_TEMP) fputs(";\n", fp);
    break;
  case IR_UNBIND: {
    /* Lepas binding loop implisit bila syarat (probe awal) truthy. */
    IRValue *ut = i->data.unbind.target;
    if (ut && ut->data.name) {
      fputs("  rupa_unbind(env, ", fp);
      cstr(fp, ut->data.name);
      fputs(", ", fp);
      if (i->data.unbind.condition) emitValue(g, NULL, i->data.unbind.condition, env);
      else fputs("valueBoolean(true)", fp);
      fputs(");\n", fp);
    }
    break;
  }
  case IR_MARK_PUB: {
    /* `pub x = 10` / `pub f() {}` — binding publik (design fn). */
    IRValue *mt = i->data.mark_pub.target;
    if (mt && mt->data.name) {
      fputs("  semMarkPub(env, ", fp);
      cstr(fp, mt->data.name);
      fputs(");\n", fp);
    }
    break;
  }
  case IR_STRSLOT_GET: case IR_STRSLOT_SET:
    fail(g, "string-slot IR is not yet supported by the C backend"); break;
  case IR_CAST:
    fail(g, "IR_CAST is not yet supported by the C backend"); break;
  case IR_INTERP:
    fail(g, "IR_INTERP requires AST/runtime interpreter and cannot be compiled yet"); break;
  default:
    fail(g, "unsupported IR opcode %d", (int)i->op); break;
  }
}

static void emitHelpers(CGen *g) {
  FILE *fp = g->fp;
  fputs("#include <rupa.h>\n\n", fp);
  fputs("static RuntimeValue rupa_get(RuntimeEnv *env, const char *name) { RuntimeValue v=valueNull(); semGet(env,name,&v); return v; }\n", fp);
  fputs("static void rupa_set(RuntimeEnv *env,const char *name,RuntimeValue v,bool c){ if(c) semSetConst(env,name,v); else semSet(env,name,v); }\n", fp);
  fputs("static RuntimeValue rupa_binary(const char *op,RuntimeValue a,RuntimeValue b,Error *e){ bool ok=false; RuntimeValue v=valueBinaryApply(op,a,b,&ok); if(!ok&&e) addRuntimeError(e,ERR_TYPE_MISMATCH,op,\"invalid operands\"); return v; }\n", fp);
  fputs("static RuntimeValue rupa_neg(RuntimeValue v){ if(v.type==VALUE_NUMBER)return valueNumber(-v.as.number); if(v.type==VALUE_DECIMAL)return valueDecimal(-v.as.decimal); return valueNull(); }\n", fp);
  fputs("static RuntimeValue valueCompare(const char *op,RuntimeValue a,RuntimeValue b){ bool r=false; if(!strcmp(op,\"==\"))r=valueEquals(a,b); else if(!strcmp(op,\"!=\"))r=!valueEquals(a,b); else if((a.type==VALUE_NUMBER||a.type==VALUE_DECIMAL)&&(b.type==VALUE_NUMBER||b.type==VALUE_DECIMAL)){double x=a.type==VALUE_DECIMAL?a.as.decimal:(double)a.as.number,y=b.type==VALUE_DECIMAL?b.as.decimal:(double)b.as.number; if(!strcmp(op,\"<\"))r=x<y; else if(!strcmp(op,\"<=\"))r=x<=y; else if(!strcmp(op,\">\"))r=x>y; else if(!strcmp(op,\">=\"))r=x>=y;} return valueBoolean(r); }\n", fp);
  fputs("static RuntimeValue rupa_call(RuntimeValue callee,RuntimeEnv *env,Error *e,size_t n,RuntimeValue *a){ if(callee.type==VALUE_NATIVE_FUNCTION&&callee.as.nativeFunc){ struct RuntimeNativeFunction *f=callee.as.nativeFunc; int argc=(int)n+(f->hasReceiver?1:0); RuntimeValue *argv=calloc(argc?argc:1,sizeof(*argv)); int off=0; if(f->hasReceiver){argv[0]=f->receiver?*f->receiver:valueNull();off=1;} for(size_t i=0;i<n;i++)argv[i+off]=a[i]; InterpreterResult r=f->func(argc,argv,env,e); free(argv); return r.value;} if(e)addRuntimeError(e,ERR_INVALID_CALL,\"function\",\"value is not callable\"); return valueNull(); }\n", fp);
  fputs("static RuntimeValue rupa_call_named(const char *name,RuntimeEnv *env,Error *e,size_t n,RuntimeValue *a){ RuntimeValue fn=valueNull(); if(name)semGet(env,name,&fn); if(fn.type==VALUE_NATIVE_FUNCTION)return rupa_call(fn,env,e,n,a); if(name&&!strcmp(name,\"print\")){bool streamed=false;printRenderArgs(a,(int)n,env,e,stdout,false,&streamed);return valueNull();} if(e)addRuntimeError(e,ERR_INVALID_CALL,\"function\",name?name:\"value is not callable\"); return valueNull(); }\n", fp);
  fputs("static RuntimeValue rupa_member_get(RuntimeValue o,const char *k,Error *e){ RuntimeValue v=valueNull(); if(o.type==VALUE_OBJECT)valueObjectGet(o,k,&v); else if(o.type==VALUE_ARRAY&&!strcmp(k,\"length\"))v=valueNumber(o.as.array.length); else if(o.type==VALUE_STRING&&!strcmp(k,\"length\"))v=valueNumber(o.as.string?(long long)strlen(o.as.string):0); else if(e)addRuntimeError(e,ERR_TYPE_MISMATCH,\"object\",\"member access on non-object\"); return v; }\n", fp);
  fputs("static RuntimeValue rupa_member_set(RuntimeValue o,const char *k,RuntimeValue v,Error *e){ if(o.type==VALUE_OBJECT){valueObjectSet(&o,k,v);return o;} if(e)addRuntimeError(e,ERR_TYPE_MISMATCH,\"object\",\"member assignment on non-object\"); return o; }\n", fp);
  fputs("static RuntimeValue rupa_index_get(RuntimeValue a,RuntimeValue i,Error *e){ if(a.type==VALUE_ARRAY&&i.type==VALUE_NUMBER&&i.as.number>=0&&i.as.number<a.as.array.length)return a.as.array.items[i.as.number]; if(a.type==VALUE_PTR){bool fatal=false; RuntimeValue v=memoryPtrGet(a,i,e,&fatal);return v;} return valueNull(); }\n", fp);
  fputs("static RuntimeValue rupa_index_set(RuntimeValue a,RuntimeValue i,RuntimeValue v,Error *e){ if(a.type==VALUE_ARRAY&&i.type==VALUE_NUMBER&&i.as.number>=0){long long n=i.as.number;if(n<a.as.array.length)a.as.array.items[n]=v;else if(n==a.as.array.length){RuntimeValue *x=gcrealloc(a.as.array.items,sizeof(RuntimeValue)*(size_t)(n+1));if(x){x[n]=v;}}}else if(a.type==VALUE_PTR){bool fatal=false;memoryPtrSet(a,i,v,e,&fatal);} return a; }\n", fp);
  fputs("static RuntimeValue rupa_alloc_array(RuntimeValue n){int len=n.type==VALUE_NUMBER&&n.as.number>0?(int)n.as.number:0;RuntimeValue v=valueNull();v.type=VALUE_ARRAY;v.as.array.length=len;v.as.array.items=len?gccalloc((size_t)len,sizeof(RuntimeValue)):NULL;return v;}\n", fp);
  fputs("static RuntimeValue rupa_alloc_ptr(RuntimeValue n,int elemSize,const char *type){long long count=n.type==VALUE_NUMBER&&n.as.number>0?n.as.number:1;if(elemSize<=0)elemSize=1;void *p=gccalloc((size_t)count,(size_t)elemSize);if(type&&*type)gcregsettype(p,type);return valuePtr(p);}\n", fp);
  fputs("static RuntimeValue rupa_realloc(RuntimeValue p,RuntimeValue n){if(p.type!=VALUE_PTR||n.type!=VALUE_NUMBER)return valueNull();return valuePtr(gcrealloc(p.as.ptr,(size_t)n.as.number));}\n", fp);
  fputs("static void rupa_free(RuntimeValue p){if(p.type==VALUE_PTR)gcfree(p.as.ptr);}\n", fp);
  fputs("static void rupa_check(RuntimeValue v,const char *type,const char *fn,Error *e){if(fn){analyzerCheckReturnType(fn,type,v,e);}else if(v.type==VALUE_PTR){memoryHandleTypeCheck(v,type,e);}else analyzerCheckType(type,v,e);}\n", fp);
  fputs("static RuntimeValue rupa_absent(RuntimeEnv *env,const char *name){RuntimeValue v=valueNull();return valueBoolean(!semGet(env,name,&v));}\n", fp);
  fputs("static void rupa_unbind(RuntimeEnv *env,const char *name,RuntimeValue cond){if(valueTruthy(cond))semUnsetLocal(env,name);}\n\n", fp);
}

static void emitPrototypes(CGen *g) {
  FILE *fp=g->fp;
  for(size_t i=0;i<g->functions.count;i++){
    fputs("static RuntimeValue rupa_fn_",fp); ident(fp,g->functions.items[i]->name); fputs("(RuntimeEnv *parent, Error *error, size_t argc, RuntimeValue *argv);\n",fp);
  }
  fputc('\n',fp);
}

static void emitFunction(CGen *g, const IRFunction *fn) {
  FILE *fp=g->fp;
  fprintf(fp,"static RuntimeValue rupa_fn_"); ident(fp,fn->name);
  fputs("(RuntimeEnv *parent, Error *error, size_t argc, RuntimeValue *argv) {\n",fp);
  fputs("  RuntimeEnv *env = semCreateEnv(parent);\n  if (!env) return valueNull();\n",fp);
  for(size_t p=0;p<fn->param_count;p++){
    IRValue *v=fn->params[p];
    if(v&&v->data.name){ fprintf(fp,"  if (argc > %zu) semSet(env, ",p); cstr(fp,v->data.name); fprintf(fp,", argv[%zu]);\n",p); }
  }
  for(size_t i=0;i<g->temps.count;i++) fprintf(fp,"  RuntimeValue %s = valueNull();\n",tempName(g->temps.items[i]));
  for(IRBlock *b=fn->first_block;b;b=b->next){
    fprintf(fp,"block_%u:\n",b->id);
    for(IRInstruction *i=b->first;i;i=i->next){ emitInstruction(g,i,"env","error"); if(g->unsupported) return; }
  }
  fputs("  return valueNull();\n}\n\n",fp);
}

static int generateC(IRModule *ir, const char *path) {
  CGen g={0};
  g.fp=fopen(path,"w");
  if(!g.fp){fprintf(stderr,"rupa: cannot write %s: %s\n",path,strerror(errno));return 1;}
  collectModule(&g,ir);
  if(!g.unsupported) emitHelpers(&g);
  if(!g.unsupported) emitPrototypes(&g);
  for(size_t i=0;i<g.functions.count&&!g.unsupported;i++) emitFunction(&g,g.functions.items[i]);
  if(!g.unsupported){
    fputs("int main(void) {\n  gcinit(100);\n  RuntimeEnv *env=semCreateEnv(NULL);\n  if(!env){gcclean();return 1;}\n  stdlibInit(env);\n  builtinsInit(env);\n  Error *error=createError(16);\n  RuntimeValue none=valueNull();\n  (void)none;\n",g.fp);
    bool hasMain=false;
    for(size_t i=0;i<g.functions.count;i++) if(!strcmp(g.functions.items[i]->name,"main")){hasMain=true;break;}
    if(hasMain) fputs("  rupa_fn_main(env,error,0,NULL);\n",g.fp);
    fputs("  int status=(error&&error->size)?1:0; if(error&&error->size) printErrors(error); gcclean(); return status;\n}\n",g.fp);
  }
  fclose(g.fp); free(g.temps.items); free(g.functions.items);
  if(g.unsupported){unlink(path);fprintf(stderr,"rupa: compile: %s\n",g.reason);return 1;}
  return 0;
}

static void shellQuote(char *out, size_t cap, const char *s) {
  size_t w = 0;
  if (!cap) return;
  out[w++] = '\'';
  if (s) {
    for (; *s && w + 5 < cap; s++) {
      if (*s == '\'') { out[w++] = '\''; out[w++] = '\\'; out[w++] = '\''; out[w++] = '\''; }
      else out[w++] = *s;
    }
  }
  if (w + 1 < cap) out[w++] = '\'';
  out[w] = '\0';
}

/* Root project rupa — die-resolve SEKALI dari lokasi binary.
 * Compile `-o` harus jalan dari direktori mana pun: header publik
 * (include/rupa.h), intl.h (via -I.), dan librupa.a semuanya relatif
 * ke root project, bukan cwd user. Menunjuk ke binary lama/replika
 * tanpa tree project = error jelas, bukan header setengah jalan
 * (implicit declaration semUnsetLocal dsb). */
static const char *codegenRoot(void) {
  static char root[4096];
  static bool resolved = false;
  if (resolved) return root[0] ? root : NULL;
  resolved = true;

#if CODEGEN_HAS_READLINK
  char exe[4096];
  ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (n <= 0) return NULL;
  exe[n] = '\0';

  /* Binary ada di <root>/bin/<name> — naik satu level. */
  char *slash = strrchr(exe, '/');
  if (!slash) return NULL;
  *slash = '\0';
  slash = strrchr(exe, '/');
  if (!slash) return NULL;
  size_t len = (size_t)(slash - exe);
  if (len == 0 || len >= sizeof(root)) return NULL;
  memcpy(root, exe, len);
  root[len] = '\0';

  /* Sanity: tree project wajib punya include/rupa.h + lib/librupa.a. */
  char probe[4200];
  snprintf(probe, sizeof(probe), "%s/include/rupa.h", root);
  struct stat st;
  if (stat(probe, &st) != 0) {
    root[0] = '\0';
    return NULL;
  }
  return root;
#else
  (void)root;
  return NULL; /* platform tanpa /proc: set RUPA_LIB + header manual */
#endif
}

static int runCC(const char *sourceC, const char *output) {
  const char *root = codegenRoot();
  if (!root) {
    fprintf(stderr,
            "rupa: compile: project root tidak ditemukan (binary harus "
            "berada di <root>/bin/ dengan include/rupa.h + lib/librupa.a)\n");
    return 1;
  }

  char inc[4200], incDot[4200], lib[4200];
  snprintf(inc, sizeof(inc), "-I%s/include", root);
  snprintf(incDot, sizeof(incDot), "-I%s", root);
  snprintf(lib, sizeof(lib), "%s/lib/librupa.a", root);

  /* RUPA_LIB menimpa default librupa.a (path absolut atau relatif cwd). */
  const char *libEnv = getenv("RUPA_LIB");
  if (libEnv && *libEnv) snprintf(lib, sizeof(lib), "%s", libEnv);

  const char *cc = getenv("CC");
  if (!cc || !*cc) cc = "cc";
  char qSource[1200], qLib[4400], qOutput[1200], cmd[8192];
  shellQuote(qSource, sizeof(qSource), sourceC);
  shellQuote(qLib, sizeof(qLib), lib);
  shellQuote(qOutput, sizeof(qOutput), output);
  snprintf(cmd, sizeof(cmd), "%s -std=gnu11 %s %s %s %s -lm -lpthread -lssl -lcrypto -o %s", cc,
           inc, incDot, qSource, qLib, qOutput);
  return system(cmd) == 0 ? 0 : 1;
}

int compileIR(IRModule *ir, const char *output) {
  if (!ir || !output || !*output) {
    fprintf(stderr, "rupa: compile: IR and output are required\n");
    return 1;
  }

  char tmp[2048];
  snprintf(tmp, sizeof(tmp), "%s.rupa.gen.c", output);

  int rc = generateC(ir, tmp);
  if (rc == 0) rc = runCC(tmp, output);

  remove(tmp);
  return rc;
}
