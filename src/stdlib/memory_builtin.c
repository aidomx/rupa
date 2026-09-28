#include <rupa.h>

/* memory_builtin.c — del (free variadic/array) + builtin dupl/compare
 * (design/str_memory.txt) + memoryInit. Unit hasil split memory.c:
 *   memory_new.c     — new/Contract (alokasi type-driven)
 *   memory_index.c   — indexing VALUE_PTR (get/set interpreter+IR)
 *   memory_member.c  — member access struct + string slot
 */

/* ==================== del ==================== */

/* del(handle) — satu handle; return null. Guard: ptr milik GC;
 * view handle ditolak (kepemilikan ada di blok owner). */
static InterpreterResult delHandle(RuntimeValue v, Error *error) {
  if (v.type != VALUE_PTR || !v.as.ptr) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "del(x)", "expects a ptr (GC-owned)");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (gcregisview(v.as.ptr)) {
    addRuntimeError(error, ERR_MEMORY, "del(x)",
                    "handle is a view into a struct block — delete the owner instead");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  if (gcfind(v.as.ptr) < 0) {
    addRuntimeError(error, ERR_MEMORY, "del(x)", "pointer not owned by GC");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  gcfree(v.as.ptr);
  return resultNormal(valueNull());
}

/* del(x, y, ...) / del([x, y]) — free variadic / dari array. */
InterpreterResult memoryDelCall(Node *node, AstNode *ast, RuntimeEnv *env, Error *error,
                                bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL) return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  const char *name = NULL;
  if (calleeAst->type == NODE_IDENTIFIER)
    name = calleeAst->identifier.name;
  else if (calleeAst->type == NODE_LITERAL_ID)
    name = calleeAst->string.value;
  if (!name || strcmp(name, "del") != 0) return resultNormal(valueNull());

  *handled = true;
  if (ast->call.length < 1) return resultNormal(valueNull());

  for (int i = 0; i < ast->call.length; i++) {
    InterpreterResult r = interpretNode(node, ast->call.args[i], env, error);
    if (r.flow != FLOW_NORMAL) return r;

    if (r.value.type == VALUE_ARRAY) {
      /* del([x, y]) — tiap elemen array adalah handle. */
      for (int j = 0; j < r.value.as.array.length; j++) {
        InterpreterResult d = delHandle(r.value.as.array.items[j], error);
        if (d.flow != FLOW_NORMAL) return d;
      }
      continue;
    }
    InterpreterResult d = delHandle(r.value, error);
    if (d.flow != FLOW_NORMAL) return d;
  }
  return resultNormal(valueNull());
}

/* ==================== dupl / compare ==================== */

/* dupl(str) — strdup GC-tracked; dupl(str, n) — strndup maksimal n
 * char, selalu NUL-terminated. Menggantikan dupin/maxdupin. */
static InterpreterResult builtinDupl(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "dupl(str)", "expects a string");
    return resultFlow(FLOW_ERROR, valueNull());
  }
  const char *s = argv[0].as.string ? argv[0].as.string : "";
  char *copy = NULL;
  if (argc >= 2) {
    if (argv[1].type != VALUE_NUMBER || argv[1].as.number < 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "dupl(str, n)", "n must be a non-negative number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    copy = gcstrndup(s, (size_t)argv[1].as.number);
  } else {
    copy = gcstrdup(s);
  }
  return resultNormal(valuePtr(copy));
}

/* compare(a, b) — strcmp-style untuk string (handle slot & string
 * biasa); compare(a, b, n) — memcmp untuk blok mentah. Menggantikan
 * pincmp untuk kasus string. */
static InterpreterResult builtinCompare(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                        Error *error) {
  (void)env;
  if (argc < 2) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b)", "expects two string/ptr arguments");
    return resultFlow(FLOW_ERROR, valueNull());
  }

  const char *as = NULL;
  const char *bs = NULL;
  if (argv[0].type == VALUE_STRING)
    as = argv[0].as.string;
  else if (argv[0].type == VALUE_PTR && argv[0].as.ptr && gcfind(argv[0].as.ptr) >= 0)
    as = (const char *)argv[0].as.ptr;
  if (argv[1].type == VALUE_STRING)
    bs = argv[1].as.string;
  else if (argv[1].type == VALUE_PTR && argv[1].as.ptr && gcfind(argv[1].as.ptr) >= 0)
    bs = (const char *)argv[1].as.ptr;
  if (!as || !bs) {
    addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b)",
                    "expects strings or GC-owned handles");
    return resultFlow(FLOW_ERROR, valueNull());
  }

  if (argc >= 3) {
    if (argv[2].type != VALUE_NUMBER || argv[2].as.number < 0) {
      addRuntimeError(error, ERR_TYPE_MISMATCH, "compare(a, b, n)",
                      "n must be a non-negative number");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    return resultNormal(valueNumber(gccmp(as, bs, (size_t)argv[2].as.number)));
  }
  return resultNormal(valueNumber(strcmp(as, bs)));
}

/* ==================== register ==================== */

void memoryInit(RuntimeEnv *env) {
  if (!env) return;
  /* new/del tidak di-register sebagai native function biasa:
   * arg pertamanya nama tipe (tidak dievaluasi), jadi di-intercept
   * di interpretCall via memoryNewCall/memoryDelCall. */
  /* dupl/compare (design/str_memory.txt) — global, tanpa import. */
  semSet(env, "dupl", valueNativeFunction("dupl", builtinDupl, 1));
  semSet(env, "compare", valueNativeFunction("compare", builtinCompare, 2));
}
