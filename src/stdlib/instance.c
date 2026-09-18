#include <rupa.h>

/* instance.c — instantiation class (design/new_class.txt): `new Counter(data)`.
 *
 * Hook di interpretCall, pola yang sama dengan memoryNewCall (new/del),
 * tetapi TIDAK ada kaitan dengan manajemen memori: class adalah object
 * runtime ( RuntimeObject ), bukan blok memori type-driven. Karena itu
 * modul ini terpisah dari memory.c.
 *
 *   new Counter()         — instance baru, construct() dipanggil tanpa args
 *                           (args di-bind sebagai array kosong bila
 *                           construct punya parameter).
 *   new Counter({v: 1})   — instance baru + construct(args) dengan args =
 *                           array berisi value argumen ke-2..N.
 *
 * Arg[0] new SELALU nama (tidak dievaluasi) — di sini nama CLASS terdaftar
 * (NODE_CLASS_DECL). Bukan class → *handled = false, jalur new memori
 * (Contract, Number, dst.) lanjut seperti biasa. Struct murni tidak bisa
 * di-instantiate. */

/* Salin entries object prototype (method + field default) — append agar
 * urutan stabil dan mutasi valueObjectSet terlihat di semua salinan. */
static struct RuntimeObjectEntry *instanceCopyEntries(
    struct RuntimeObjectEntry *src) {
  struct RuntimeObjectEntry *copy = NULL;
  struct RuntimeObjectEntry *tail = NULL;
  for (struct RuntimeObjectEntry *e = src; e; e = e->next) {
    struct RuntimeObjectEntry *n = gccalloc(1, sizeof(*n));
    if (!n) break;
    n->key = gcstrdup(e->key);
    n->value = e->value;
    n->next = NULL;
    if (!copy) {
      copy = n;
    } else {
      tail->next = n;
    }
    tail = n;
  }
  return copy;
}

InterpreterResult instanceNewCall(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error, bool *handled) {
  *handled = false;
  if (!node || !ast || ast->type != NODE_CALL || !env)
    return resultNormal(valueNull());

  AstNode *calleeAst = &node->ast[ast->call.callee];
  const char *name = NULL;
  if (calleeAst->type == NODE_IDENTIFIER)
    name = calleeAst->identifier.name;
  else if (calleeAst->type == NODE_LITERAL_ID)
    name = calleeAst->string.value;
  if (!name || strcmp(name, "new") != 0) return resultNormal(valueNull());

  if (ast->call.length < 1 || ast->call.args[0] < 0 ||
      ast->call.args[0] >= node->length)
    return resultNormal(valueNull());

  /* Arg[0] = nama class, tidak dievaluasi (sejajar new T() memori). */
  AstNode *typeAst = &node->ast[ast->call.args[0]];
  const char *clsName = NULL;
  if (typeAst->type == NODE_IDENTIFIER)
    clsName = typeAst->identifier.name;
  else if (typeAst->type == NODE_LITERAL_ID)
    clsName = typeAst->string.value;
  if (!clsName || !analyzerIsClass(clsName)) return resultNormal(valueNull());

  /* Prototype = instance default yang ter-bind ke nama class sejak
   * deklarasi (interpretClass). Harus object dengan minimal satu method
   * — kalau bukan, bukan class yang valid untuk instantiation. */
  RuntimeValue proto = valueNull();
  if (!semGet(env, clsName, &proto) || proto.type != VALUE_OBJECT ||
      !proto.as.object.entries)
    return resultNormal(valueNull());
  bool hasMethod = false;
  for (struct RuntimeObjectEntry *e = proto.as.object.entries; e; e = e->next) {
    if (e->value.type == VALUE_FUNCTION) {
      hasMethod = true;
      break;
    }
  }
  if (!hasMethod) return resultNormal(valueNull());

  *handled = true;
  RuntimeValue fresh = valueObject(instanceCopyEntries(proto.as.object.entries));

  /* args = value argumen ke-2..N sebagai array; tanpa args → array kosong
   * (construct(args: unknown[]) tetap bisa baca args.length == 0). */
  int argc = ast->call.length - 1;
  RuntimeValue args = valueArray(NULL, 0);
  if (argc > 0) {
    RuntimeValue *items = gccalloc((size_t)argc, sizeof(RuntimeValue));
    if (!items) {
      addRuntimeError(error, ERR_INTERNAL, "new ClassName()",
                      "out of memory building args");
      return resultFlow(FLOW_ERROR, valueNull());
    }
    for (int i = 0; i < argc; i++) {
      InterpreterResult r =
          interpretNode(node, ast->call.args[i + 1], env, error);
      if (r.flow != FLOW_NORMAL) return r;
      items[i] = r.value;
    }
    args = valueArray(items, argc);
  }

  classRunConstruct(node, fresh, env, error, args);
  /* Lifecycle @created: method ber-marker dipanggil saat instance baru
   * pertama kali dipakai (dibuat via new). */
  classRunLifecycle(node, fresh, env, error, args);
  return resultNormal(fresh);
}
