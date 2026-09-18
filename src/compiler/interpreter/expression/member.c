#include <rupa.h>

/* this.get("key") — akses field generic pada object penerima (argv[0]). */
static InterpreterResult stdObjectGet(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                      Error *error) {
  (void)env;
  (void)error;
  if (argc < 2 || argv[0].type != VALUE_OBJECT ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return resultNormal(valueNull());
  RuntimeValue out = valueNull();
  valueObjectGet(argv[0], argv[1].as.string, &out);
  return resultNormal(out);
}

/* Hapus anchor implisit (key "") dari hasil — dipakai valueObjectSet
 * consumer yang tidak boleh melihat metadata list. */

/* this.set({k: v, ...}) — tulis field ke object penerima (argv[0]).
 * Object literal argumen menentukan field yang ditulis; set(null) pada
 * field = kosongkan. Return: object penerima (chainable). */
static InterpreterResult stdObjectSet(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                      Error *error) {
  (void)env;
  (void)error;
  if (argc < 2 || argv[0].type != VALUE_OBJECT || argv[1].type != VALUE_OBJECT)
    return resultNormal(argv[0]);
  for (struct RuntimeObjectEntry *e = argv[1].as.object.entries; e; e = e->next) {
    if (!e->key || !*e->key) continue; /* skip anchor implisit */
    valueObjectSet(&argv[0], e->key, e->value);
  }
  return resultNormal(argv[0]);
}

static const char *memberName(Node *node, int id) {
  if (!node || id < 0 || id >= node->length) return NULL;
  AstNode *ast = &node->ast[id];
  if (ast->type == NODE_IDENTIFIER) return ast->identifier.name;
  if (ast->type == NODE_LITERAL_ID) return ast->string.value;
  return NULL;
}

InterpreterResult interpretMember(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_MEMBER) return resultNormal(valueNull());

  /* super dispatch (design/new_class.txt langkah 3): `super.method(...)`
   * atau `super.field` di dalam method class — receiver `super` di-bind
   * interpretCall sebagai prototype class induk. Kalau binding itu tak
   * ada (bukan member call / tanpa extends), fallback: this → instance
   * dengan lookup method yang DILEWATI override anak (method anak
   * ditandai override — lihat interpretCall). */
  if (ast->member.object >= 0 && ast->member.object < node->length) {
    AstNode *o = &node->ast[ast->member.object];
    if (o->type == NODE_IDENTIFIER && o->identifier.name &&
        !strcmp(o->identifier.name, "super")) {
      RuntimeValue sp = valueNull();
      if (semGet(env, "super", &sp) && sp.type == VALUE_OBJECT) {
        const char *key = memberName(node, ast->member.member);
        RuntimeValue val = valueNull();
        if (key && valueObjectGet(sp, key, &val))
          return resultNormal(val);
        return resultNormal(valueNull());
      }
      /* Tidak ada binding super (mis. dipakai di luar method) —
       * error yang jelas. */
      if (error)
        addError(error, (ErrorInfo){.code = "ReferenceError",
                                    .message = "super requires a class that extends",
                                    .line = 0, .row = 0,
                                    .type = ERR_UNDEFINED_VAR});
      return resultFlow(FLOW_ERROR, valueNull());
    }
  }

  InterpreterResult obj = interpretNode(node, ast->member.object, env, error);
  if (obj.flow != FLOW_NORMAL) return obj;

  const char *key = memberName(node, ast->member.member);

  /* Struct handle ptr (C3): field access via layout offset. */
  if (obj.value.type == VALUE_PTR && obj.value.as.ptr && key) {
    RuntimeValue fieldOut = valueNull();
    bool fatal = false;
    if (memoryMemberGet(obj.value.as.ptr, key, &fieldOut, error, &fatal))
      return resultNormal(fieldOut);
    if (fatal) return resultFlow(FLOW_ERROR, valueNull());
    /* bukan struct handle — jalur lama lanjut */
  }

  if (obj.value.type == VALUE_OBJECT) {
    RuntimeValue val;
    if (valueObjectGet(obj.value, key, &val)) return resultNormal(val);
    /* this.get("key") / this.set({...}) — accessor generic pada this
     * (design/new_class.txt contoh). get: argumen nama field; set:
     * object literal field yang ditulis. */
    if (key && (!strcmp(key, "get") || !strcmp(key, "set"))) {
      RuntimeValue method = valueNativeFunction(
          key, !strcmp(key, "get") ? stdObjectGet : stdObjectSet, 1);
      method.as.nativeFunc->hasReceiver = true;
      method.as.nativeFunc->receiver = gcmall(sizeof(RuntimeValue));
      if (method.as.nativeFunc->receiver) {
        *method.as.nativeFunc->receiver = obj.value;
        /* Write-back: receiver object disalin by value — set() menambah
         * field via anchor di salinan. Simpan nama binding receiver bila
         * ada (o.set / this.set) supaya native bisa semSet kembali.
         * this binding: this di frame method sudah menunjuk instance
         * yang sama (tail shared), jadi cukup untuk nama top-level. */
        if (ast->member.object >= 0 && ast->member.object < node->length) {
          AstNode *bo = &node->ast[ast->member.object];
          const char *bn = bo->type == NODE_IDENTIFIER     ? bo->identifier.name
                           : bo->type == NODE_LITERAL_ID ? bo->string.value
                                                         : NULL;
          if (bn) {
            RuntimeValue probe = valueNull();
            if (strcmp(bn, "this") == 0 || semGet(env, bn, &probe)) {
              method.as.nativeFunc->bindingName = gcstrdup(bn);
              method.as.nativeFunc->bindingEnv = env;
            }
          }
        }
      }
      return resultNormal(method);
    }
    return resultNormal(valueNull());
  }

  /* Array .length property */
  if (obj.value.type == VALUE_ARRAY && key && !strcmp(key, "length"))
    return resultNormal(valueNumber(obj.value.as.array.length));

  /* String .length property and bound methods */
  if (obj.value.type == VALUE_STRING && key) {
    if (!strcmp(key, "length"))
      return resultNormal(valueNumber(obj.value.as.string ? (int)strlen(obj.value.as.string) : 0));

    NativeFn fn = NULL;
    int paramCount = 0;
    if (!strcmp(key, "upper")) {
      fn = stdStringUpper;
      paramCount = 0;
    } else if (!strcmp(key, "lower")) {
      fn = stdStringLower;
      paramCount = 0;
    } else if (!strcmp(key, "trim")) {
      fn = stdStringTrim;
      paramCount = 0;
    } else if (!strcmp(key, "contains")) {
      fn = stdStringContains;
      paramCount = 1;
    } else if (!strcmp(key, "startsWith")) {
      fn = stdStringStartsWith;
      paramCount = 1;
    } else if (!strcmp(key, "endsWith")) {
      fn = stdStringEndsWith;
      paramCount = 1;
    } else if (!strcmp(key, "replace")) {
      fn = stdStringReplace;
      paramCount = 2;
    } else if (!strcmp(key, "split")) {
      fn = stdStringSplit;
      paramCount = 1;
    } else if (!strcmp(key, "indexOf")) {
      fn = stdStringIndexOf;
      paramCount = 1;
    } else if (!strcmp(key, "slice")) {
      fn = stdStringSlice;
      paramCount = 2;
    }
    if (fn) {
      RuntimeValue method = valueNativeFunction(key, fn, paramCount);
      method.as.nativeFunc->hasReceiver = true;
      method.as.nativeFunc->receiver = gcmall(sizeof(RuntimeValue));
      if (method.as.nativeFunc->receiver) *method.as.nativeFunc->receiver = obj.value;
      return resultNormal(method);
    }
  }

  if (obj.value.type == VALUE_NULL) return resultNormal(valueNull());

  static char message[256];
  snprintf(message, sizeof(message), "cannot access property '%s' on value of type '%s'",
           key ? key : "?", valueTypeName(obj.value.type));
  if (error)
    addError(error, (ErrorInfo){.code = "TypeError",
                                .message = gcdup(message),
                                .line = 0,
                                .row = 0,
                                .type = ERR_TYPE_MISMATCH});
  return resultFlow(FLOW_ERROR, valueNull());
}

/*
 * Member assignment: obj.field = value or arr[i] = value.
 *
 * The target AST node is either NODE_MEMBER or NODE_SUBSCRIPT.
 * For objects: update the entry in-place via valueObjectSet.
 * For arrays:  replace the element at the given index.
 */
InterpreterResult interpretMemberAssign(Node *node, AstNode *ast, RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_MEMBER_ASSIGN) return resultNormal(valueNull());

  AstNode *target = &node->ast[ast->memberAssign.target];

  /* Evaluate the value to assign. */
  InterpreterResult val = interpretNode(node, ast->memberAssign.value, env, error);
  if (val.flow != FLOW_NORMAL) return val;

  /* --- Object member assignment: obj.field = value --- */
  if (target->type == NODE_MEMBER) {
    /* Resolve the base object (not the leaf member). */
    InterpreterResult base = interpretNode(node, target->member.object, env, error);
    if (base.flow != FLOW_NORMAL) return base;

    const char *key = memberName(node, target->member.member);

    /* Struct handle ptr (C3): field write via layout offset. */
    if (base.value.type == VALUE_PTR && base.value.as.ptr && key) {
      bool fatal = false;
      if (memoryMemberSet(base.value.as.ptr, key, val.value, error, &fatal)) {
        return resultNormal(val.value);
      }
      if (fatal) return resultFlow(FLOW_ERROR, valueNull());
      /* bukan struct handle — jalur lama lanjut */
    }

    if (base.value.type == VALUE_OBJECT) {
      if (!valueObjectSet(&base.value, key, val.value)) {
        if (error)
          addError(error, (ErrorInfo){.code = "InternalError",
                                      .message = "failed to set object property",
                                      .line = 0,
                                      .row = 0,
                                      .type = ERR_INTERNAL});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      /* Write the mutated object back to the environment so the
       * assignment is visible to subsequent lookups. */
      if (target->member.object >= 0 && target->member.object < node->length) {
        AstNode *baseAst = &node->ast[target->member.object];
        const char *baseName = NULL;
        if (baseAst->type == NODE_IDENTIFIER)
          baseName = baseAst->identifier.name;
        else if (baseAst->type == NODE_LITERAL_ID)
          baseName = baseAst->string.value;
        if (baseName) semSet(env, baseName, base.value);
      }
      return resultNormal(val.value);
    }

    static char message[256];
    snprintf(message, sizeof(message), "cannot assign property '%s' on value of type '%s'",
             key ? key : "?", valueTypeName(base.value.type));
    if (error)
      addError(error, (ErrorInfo){.code = "TypeError",
                                  .message = message,
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return resultFlow(FLOW_ERROR, valueNull());
  } /* --- Object element assignment: obj["key"] = value --- */
  if (target->type == NODE_SUBSCRIPT) {
    InterpreterResult base = interpretNode(node, target->subscript.posId, env, error);
    if (base.flow != FLOW_NORMAL) return base;

    if (base.value.type == VALUE_OBJECT) {
      InterpreterResult keyRes = interpretNode(node, target->subscript.index, env, error);
      if (keyRes.flow != FLOW_NORMAL) return keyRes;
      if (keyRes.value.type != VALUE_STRING || !keyRes.value.as.string) {
        if (error)
          addError(error, (ErrorInfo){.code = "TypeError",
                                      .message = "object index must be a string",
                                      .line = 0,
                                      .row = 0,
                                      .type = ERR_TYPE_MISMATCH});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      if (!valueObjectSet(&base.value, keyRes.value.as.string, val.value)) {
        if (error)
          addError(error, (ErrorInfo){.code = "InternalError",
                                      .message = "failed to set object property",
                                      .line = 0,
                                      .row = 0,
                                      .type = ERR_INTERNAL});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      /* Write the mutated object back (mirrors obj.field = v). */
      if (target->subscript.posId >= 0 && target->subscript.posId < node->length) {
        AstNode *baseAst = &node->ast[target->subscript.posId];
        const char *baseName = NULL;
        if (baseAst->type == NODE_IDENTIFIER)
          baseName = baseAst->identifier.name;
        else if (baseAst->type == NODE_LITERAL_ID)
          baseName = baseAst->string.value;
        if (baseName) semSet(env, baseName, base.value);
      }
      return resultNormal(val.value);
    }
  }

  /* --- Array element assignment: arr[i] = value --- */
  if (target->type == NODE_SUBSCRIPT) {
    InterpreterResult base = interpretNode(node, target->subscript.posId, env, error);
    if (base.flow != FLOW_NORMAL) return base;

    /* VALUE_PTR (handle new T()) — tulis elemen via registry v2. */
    if (base.value.type == VALUE_PTR) {
      bool ptrHandled = false;
      InterpreterResult ptrResult =
          memoryIndexSet(node, target->subscript.posId, target->subscript.index,
                         val.value, env, error, &ptrHandled);
      if (ptrHandled) return ptrResult;
    }

    if (base.value.type != VALUE_ARRAY) {
      static char message[256];
      snprintf(message, sizeof(message), "cannot index into value of type '%s'",
               valueTypeName(base.value.type));
      if (error)
        addError(error, (ErrorInfo){.code = "TypeError",
                                    .message = message,
                                    .line = 0,
                                    .row = 0,
                                    .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    InterpreterResult idx = interpretNode(node, target->subscript.index, env, error);
    if (idx.flow != FLOW_NORMAL) return idx;

    if (idx.value.type != VALUE_NUMBER) {
      if (error)
        addError(error, (ErrorInfo){.code = "TypeError",
                                    .message = "array index must be a number",
                                    .line = 0,
                                    .row = 0,
                                    .type = ERR_TYPE_MISMATCH});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    int i = (int)idx.value.as.number;
    int len = base.value.as.array.length;
    /* Auto-grow: arr[len] = v menambah elemen baru (push semantics).
     * Indeks di luar itu tetap out of bounds. */
    if (i == len) {
      int newLen = len + 1;
      RuntimeValue *items = gcrealloc(base.value.as.array.items, sizeof(RuntimeValue) * newLen);
      if (!items && newLen > 0) {
        if (error)
          addError(error, (ErrorInfo){.code = "IOError",
                                      .message = "out of memory growing array",
                                      .line = 0,
                                      .row = 0,
                                      .type = ERR_INTERNAL});
        return resultFlow(FLOW_ERROR, valueNull());
      }
      items[newLen - 1] = val.value;
      base.value.as.array.items = items;
      base.value.as.array.length = newLen;
    } else if (i < 0 || i >= len) {
      static char message[256];
      snprintf(message, sizeof(message), "index %d is out of bounds for array of length %d", i,
               len);
      if (error)
        addError(error, (ErrorInfo){.code = "RangeError",
                                    .message = message,
                                    .line = 0,
                                    .row = 0,
                                    .type = ERR_INDEX_OUT_OF_BOUNDS});
      return resultFlow(FLOW_ERROR, valueNull());
    }

    base.value.as.array.items[i] = val.value;

    /* Write the mutated array back. */
    if (target->subscript.posId >= 0 && target->subscript.posId < node->length) {
      AstNode *baseAst = &node->ast[target->subscript.posId];
      const char *baseName = NULL;
      if (baseAst->type == NODE_IDENTIFIER)
        baseName = baseAst->identifier.name;
      else if (baseAst->type == NODE_LITERAL_ID)
        baseName = baseAst->string.value;
      if (baseName) semSet(env, baseName, base.value);
    }
    return resultNormal(val.value);
  }

  if (error)
    addError(error, (ErrorInfo){.code = "SyntaxError",
                                .message = "invalid assignment target",
                                .line = 0,
                                .row = 0,
                                .type = ERR_SYNTAX});
  return resultFlow(FLOW_ERROR, valueNull());
}
