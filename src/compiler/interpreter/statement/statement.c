#include <rupa.h>

static const char *nameOf(Node *n, int id) {
  return n && id >= 0 && id < n->length && n->ast[id].type == NODE_IDENTIFIER
             ? n->ast[id].identifier.name
             : NULL;
}

static bool formatType(Node *n, int id, char *buffer, size_t capacity) {
  if (!n || id < 0 || id >= n->length || !buffer || capacity == 0) return false;

  AstNode *a = &n->ast[id];
  if (a->type == NODE_IDENTIFIER) {
    int written = snprintf(buffer, capacity, "%s", a->identifier.name);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type == NODE_LITERAL_ID) {
    int written = snprintf(buffer, capacity, "%s", a->string.value);
    return written > 0 && (size_t)written < capacity;
  }
  if (a->type != NODE_ARRAY_TYPE) return false;

  char element[256];
  if (!formatType(n, a->arrayType.elementType, element, sizeof(element))) return false;
  int written = snprintf(buffer, capacity, "%s[]", element);
  return written > 0 && (size_t)written < capacity;
}

static const char *typeOf(Node *n, int id) {
  static char typeName[256];
  return formatType(n, id, typeName, sizeof(typeName)) ? typeName : NULL;
}

InterpreterResult interpretStatement(Node *n, int id, RuntimeEnv *e, Error *x) {
  if (!n || id < 0 || id >= n->length) return resultNormal(valueNull());

  AstNode *a = &n->ast[id];

  switch (a->type) {
  case NODE_FUNCTION_DECL:
    return interpretFunction(n, a, e, x);
  case NODE_ASSIGN: {
    const char *k = nameOf(n, a->assign.target);
    /* new Contract (C1–C4): di-intercept SEBELUM evaluasi value —
     * alokasi dari anotasi; tanpa anotasi = error (C1). */
    bool contractHandled = false;
    RuntimeValue contractValue = valueNull();
    bool contractOk = false;
    if (a->assign.type >= 0) {
      const char *ann = typeOf(n, a->assign.type);
      contractOk = memoryContractAssign(n, a->assign.value, ann, true, e,
                                        &contractValue, x, &contractHandled);
      if (contractHandled && !contractOk) return resultFlow(FLOW_ERROR, valueNull());
    }
    InterpreterResult r;
    if (contractOk) {
      r = resultNormal(contractValue);
    } else {
      /* const guard SEBELUM evaluasi value: assignment biasa pada
       * binding const-locked ditolak. Deklarasi const (isConst) selalu
       * lolos — re-init di setiap iterasi loop / call fungsi. */
      if (!a->assign.isConst && k && semIsConst(e, k)) {
        static char message[256];
        snprintf(message, sizeof(message),
                 "cannot reassign const variable '%s'", k);
        if (x)
          addError(x, (ErrorInfo){.code = "ConstError",
                                  .message = message,
                                  .line = n->ast[id].line,
                                  .row = n->ast[id].row,
                                  .type = ERR_TYPE_MISMATCH});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      r = interpretNode(n, a->assign.value, e, x);
      if (r.flow != FLOW_NORMAL) return r;
    }
    if (a->assign.type >= 0) {
      analyzerSetErrorLocation(n, a->assign.type);
      /* Handle VALUE_PTR: view type check via registry v3 — scalar
       * check tidak berlaku (handle opaque). */
      if (r.value.type == VALUE_PTR && !contractOk) {
        if (!memoryHandleTypeCheck(r.value, typeOf(n, a->assign.type), x))
          return resultFlow(FLOW_ERROR, valueNull());
      } else if (r.value.type != VALUE_PTR && !contractOk &&
                 !validateAnnotation(n, a->assign.type, r.value, x)) {
        return resultFlow(FLOW_ERROR, valueNull());
      }
      /* Instance new Object() lolos kontrak struct (design/object.txt):
       * stamp __type supaya set/update/delete strict terhadap layout,
       * dan assignment member strict via objectMemberWrite. */
      if (r.value.type == VALUE_OBJECT && a->assign.type >= 0) {
        const char *ann = typeOf(n, a->assign.type);
        if (ann && analyzerFindStruct(ann) && objectIsInstance(r.value))
          valueObjectSet(&r.value, "__type", valueString(ann));
      }
    }
    if (k) {
      /* Kontrak type permanen: binding menyimpan type deklarasi, dan
       * SEMUA assignment (dengan/tanpa anotasi) divalidasi terhadapnya
       * (`x: number[] = []; x = [1]`; `p: People = ...; p = {age: 1}`).
       * new T() type-driven: `x = new Number()` mencatat type `number` */
      const char *declaredType = a->assign.type >= 0 ? typeOf(n, a->assign.type) : NULL;
      if (!declaredType && r.value.type == VALUE_PTR) {
        char newType[256];
        if (memoryNewTypeName(n, a->assign.value, newType, sizeof(newType)))
          semDeclare(e, k, newType);
      }
      if (declaredType) semDeclare(e, k, declaredType);

      /* Write-through string slot (design/str_memory.txt): name = "rudi"
       * / name = dupl(...) menulis ke slot handle Contract string —
       * bukan rebind. Non-ptr & ptr tanpa tipe (dupl) dicoba; handle
       * typed lain (Contract number, dsb) langsung rebind. */
      {
        bool slotCandidate =
            r.value.type != VALUE_PTR ||
            (r.value.as.ptr && !gcregtype(r.value.as.ptr));
        RuntimeValue old;
        if (slotCandidate && semGet(e, k, &old) && old.type == VALUE_PTR && old.as.ptr) {
          if (memoryStringSlotWrite(k, old.as.ptr, r.value, x))
            return resultNormal(old);
        }
      }

      /* Reassignment handle Contract: VALUE_PTR vs declared scalar type
       * dilewati via provenance registry v3 — cek di memoryContractCheck. */
      if (r.value.type == VALUE_PTR && declaredType &&
          gcregtype(r.value.as.ptr) && strcmp(gcregtype(r.value.as.ptr), declaredType) &&
          strcmp(declaredType, "ptr")) {
        char elem[256];
        snprintf(elem, sizeof(elem), "%s", gcregtype(r.value.as.ptr));
        const char *check = declaredType;
        size_t dl = strlen(declaredType);
        if (dl >= 2 && !strcmp(declaredType + dl - 2, "[]")) {
          /* anotasi T[]: cocokkan elemen */
          char elem2[256];
          snprintf(elem2, sizeof(elem2), "%.*s", (int)(dl - 2), declaredType);
          check = elem2;
        }
        if (strcmp(elem, check)) {
          char message[512];
          snprintf(message, sizeof(message),
                   "handle of '%s' cannot be assigned to '%s'", elem, declaredType);
          addRuntimeError(x, ERR_TYPE_MISMATCH, declaredType, message);
          return resultFlow(FLOW_ERROR, valueNull());
        }
      }

      if (!validateDeclaredType(n, a->assign.value, e, k, r.value, x))
        return resultFlow(FLOW_ERROR, valueNull());
      if (a->assign.isConst) {
        /* Deklarasi const: tulis + kunci slot. Binding lama (iterasi
         * loop / call berikutnya) di-reset — deklarasi selalu menang. */
        semSetConst(e, k, r.value);
      } else {
        semSet(e, k, r.value);
      }
    }
    return r;
  }
  case NODE_CONDITIONAL_ASSIGN: {
    const char *k = nameOf(n, a->conditionalAssign.target);
    RuntimeValue old;
    if (k && semGet(e, k, &old) && valueTruthy(old)) return resultNormal(old);
    InterpreterResult r = interpretNode(n, a->conditionalAssign.value, e, x);
    if (r.flow != FLOW_NORMAL) return r;
    /* Kontrak type permanen berlaku juga di sini (x ?= v). */
    if (k && !validateDeclaredType(n, a->conditionalAssign.value, e, k, r.value, x))
      return resultFlow(FLOW_ERROR, valueNull());
    if (k) semSet(e, k, r.value);
    return r;
  }
  case NODE_ANNOTATION: {
    const char *k = nameOf(n, a->annotation.name);
    const char *type = typeOf(n, a->annotation.type);

    if (a->annotation.value < 0) {
      if (k) semDeclare(e, k, type);
      return resultNormal(valueNull());
    }

    /* new Contract (C1–C4): intercept sebelum evaluasi — alokasi dari
     * anotasi; tanpa anotasi = error (C1). */
    bool contractHandled = false;
    RuntimeValue contractValue = valueNull();
    bool contractOk = memoryContractAssign(n, a->annotation.value, type, true, e,
                                           &contractValue, x, &contractHandled);
    if (contractHandled && !contractOk) return resultFlow(FLOW_ERROR, valueNull());

    InterpreterResult r;
    if (contractOk) {
      r = resultNormal(contractValue);
    } else {
      r = interpretNode(n, a->annotation.value, e, x);
      if (r.flow != FLOW_NORMAL) return r;
    }
    analyzerSetErrorLocation(n, a->annotation.type);
    /* Handle VALUE_PTR: view type check registry v3 — tipe handle
     * tercatat saat alokasi (new T/Contract), lookup langsung. Handle
     * VALUE_PTR skip scalar check. Contract: alokasi SUDAH dari
     * anotasi — check di-skip via contractOk. */
    if (r.value.type == VALUE_PTR && !contractOk) {
      if (!memoryHandleTypeCheck(r.value, type, x))
        return resultFlow(FLOW_ERROR, valueNull());
    } else if (r.value.type != VALUE_PTR && !contractOk &&
               !validateAnnotation(n, a->annotation.type, r.value, x)) {
      return resultFlow(FLOW_ERROR, valueNull());
    }
    if (k) {
      semDeclare(e, k, type);
      semSet(e, k, r.value);
    }
    return r;
  }
  case NODE_PRINT: {
    RuntimeValue last = valueNull();

    for (int i = 0; i < a->print.length; i++) {
      InterpreterResult result = interpretNode(n, a->print.args[i], e, x);
      last = result.value;

      if (result.flow != FLOW_NORMAL) return result;

      valuePrintInterp(last, e, x);

      if (i + 1 < a->print.length) putchar(' ');
    }

    if (e->isRepl) {
      putchar('\n');
      fflush(stdout);
    }

    return resultNormal(last);
  }
  case NODE_RETURN: {
    InterpreterResult r = interpretNode(n, a->asReturn.expression, e, x);
    if (r.flow != FLOW_NORMAL) return r;
    return a->asReturn.explicitReturn ? resultFlow(FLOW_RETURN, r.value) : r;
  }
  case NODE_BLOCK: {
    RuntimeValue last = valueNull();
    for (int i = 0; i < a->block.length; i++) {
      InterpreterResult r = interpretNode(n, a->block.statements[i], e, x);
      last = r.value;
      if (r.flow == FLOW_RETURN || r.flow == FLOW_BREAK || r.flow == FLOW_CONTINUE) return r;
      /* Runtime errors are collected and do not abort the containing block. */
    }
    return resultNormal(last);
  }
  case NODE_IF: {
    InterpreterResult c = interpretNode(n, a->asIf.condition, e, x);
    if (c.flow != FLOW_NORMAL) return c;
    if (valueTruthy(c.value)) return interpretNode(n, a->asIf.thenBlock, e, x);
    if (a->asIf.elseBlock >= 0) return interpretNode(n, a->asIf.elseBlock, e, x);
    return resultNormal(valueNull());
  }
  case NODE_BREAK:
    return resultFlow(FLOW_BREAK, valueNull());
  case NODE_CONTINUE:
    return resultFlow(FLOW_CONTINUE, valueNull());
  case NODE_LOOP:
    return interpretLoop(n, a, e, x);
  case NODE_CASE:
    return interpretCase(n, a, e, x);
  case NODE_STRUCT_DECL:
    return interpretStruct(n, a, e, x);
  case NODE_ENUM_DECL:
    /* Enum (design/enum.txt): bind konstanta member + object nama enum. */
    return interpretEnum(n, a, e, x);
  case NODE_CLASS_DECL:
    /* Class (design/new_class.txt): registrasi type sama dengan struct;
     * method dalam body dikenali sebagai function decl biasa saat dipakai. */
    return interpretClass(n, a, e, x);
  case NODE_MEMBER_ASSIGN:
    return interpretMemberAssign(n, a, e, x);
  default:
    return interpretExpression(n, id, e, x);
  }
}
