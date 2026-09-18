#include <rupa.h>

/* Body struct direpresentasikan sebagai block berisi NODE_ANNOTATION
 * (name: type). Ekstrak pasangan (nama, tipe) dari block itu. */
static int extractFields(Node *node, int bodyId, struct StructField **out) {
  *out = NULL;
  if (!node || bodyId < 0 || bodyId >= node->length) return 0;

  AstNode *body = &node->ast[bodyId];
  if (body->type != NODE_BLOCK) return 0;

  int count = 0;
  struct StructField *fields = gcmall(sizeof(*fields) * (size_t)body->block.length);
  if (!fields) return 0;

  for (int i = 0; i < body->block.length; i++) {
    int sid = body->block.statements[i];
    if (sid < 0 || sid >= node->length) continue;

    AstNode *s = &node->ast[sid];
    if (s->type != NODE_ANNOTATION || s->annotation.value >= 0) continue;

    const char *fname = NULL;
    AstNode *fn = &node->ast[s->annotation.name];
    if (fn->type == NODE_IDENTIFIER)
      fname = fn->identifier.name;
    else if (fn->type == NODE_LITERAL_ID)
      fname = fn->string.value;
    if (!fname) continue;

    /* Tipe field sebagai string ("number", "Circuit[]", ...). */
    char typeName[256];
    if (!formatAstTypeName(node, s->annotation.type, typeName, sizeof(typeName)))
      continue;

    fields[count].name = gcstrdup(fname);
    fields[count].type = gcstrdup(typeName);
    count++;
  }

  *out = fields;
  return count;
}

InterpreterResult interpretStruct(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  (void)error;
  if (!node || !ast || ast->type != NODE_STRUCT_DECL)
    return resultNormal(valueNull());

  /* Struct declaration registers a type name in the environment.
   * The actual field layout is described by the body block's annotations,
   * but at runtime we only need to know the type exists so that
   * annotation validation can accept it. */
  const char *name = NULL;
  if (ast->asStruct.name >= 0 && ast->asStruct.name < node->length) {
    AstNode *n = &node->ast[ast->asStruct.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  if (name) {
    semDeclare(env, name, "struct");

    /* Daftarkan layout field ke analyzer — kontrak untuk validasi
     * annotation struct-first (design/next_struct.txt). */
    struct StructField *fields = NULL;
    int count = extractFields(node, ast->asStruct.body, &fields);
    analyzerDeclareStruct(name, fields, count);

    /* Referensi type tak dikenal di field (typo, belum dideklarasi)
     * ditolak — dilakukan SETELAH deklarasi agar self-reference
     * (`Node { next: Node[] }`) dan urutan bebas tetap valid. */
    for (int i = 0; i < count; i++) {
      if (!analyzerIsKnownType(fields[i].type)) {
        if (error) {
          setRuntimeErrorLocation(0, 0);
          char buffer[256];
          snprintf(buffer, sizeof(buffer), "unknown type '%s' for field %s",
                   fields[i].type, fields[i].name);
          addRuntimeError(error, ERR_TYPE_MISMATCH, name, buffer);
        }
        return resultFlow(FLOW_ERROR, valueNull());
      }
    }
  }

  return resultNormal(valueNull());
}

/* Class (design/new_class.txt): registrasi runtime sama dengan struct —
 * type name dikenal analyzer, layout = field di body. Method (function
 * decl) dalam body tidak dieksekusi di sini; binding terjadi saat dipakai. */
InterpreterResult interpretClass(Node *node, AstNode *ast, RuntimeEnv *env,
                                  Error *error) {
  (void)error;
  if (!node || !ast || ast->type != NODE_CLASS_DECL)
    return resultNormal(valueNull());

  const char *name = NULL;
  if (ast->asClass.name >= 0 && ast->asClass.name < node->length) {
    AstNode *n = &node->ast[ast->asClass.name];
    if (n->type == NODE_IDENTIFIER)
      name = n->identifier.name;
    else if (n->type == NODE_LITERAL_ID)
      name = n->string.value;
  }

  if (name) {
    semDeclare(env, name, "class");

    /* Layout field dari body (annotation tanpa value) — sama pola dengan
     * struct supaya validasi annotation struct-first menerima class. */
    struct StructField *fields = NULL;
    int count = extractFields(node, ast->asClass.body, &fields);
    analyzerDeclareStruct(name, fields, count);
  }

  return resultNormal(valueNull());
}
