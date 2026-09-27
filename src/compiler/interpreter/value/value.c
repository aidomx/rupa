#include <rupa.h>

RuntimeValue valueNull(void) {
  return (RuntimeValue){.type = VALUE_NULL};
}

RuntimeValue valueNumber(long long value) {
  return (RuntimeValue){.type = VALUE_NUMBER, .as.number = value};
}

RuntimeValue valueDecimal(double value) {
  return (RuntimeValue){.type = VALUE_DECIMAL, .as.decimal = value};
}

RuntimeValue valueBoolean(bool value) {
  return (RuntimeValue){.type = VALUE_BOOLEAN, .as.boolean = value};
}

RuntimeValue valueString(const char *value) {
  if (!value) return (RuntimeValue){.type = VALUE_STRING, .as.string = NULL};

  size_t length = strlen(value);
  size_t begin = 0;
  size_t end = length;
  if (length >= 2 && ((value[0] == '"' && value[length - 1] == '"') ||
                      (value[0] == '\'' && value[length - 1] == '\''))) {
    begin = 1;
    end = length - 1;
  }

  char *text = gcmall(end - begin + 1);
  if (!text) return (RuntimeValue){.type = VALUE_STRING, .as.string = NULL};

  size_t out = 0;
  for (size_t i = begin; i < end; i++) {
    if (value[i] == '\\' && i + 1 < end) {
      i++;
      switch (value[i]) {
      case 'n':
        text[out++] = '\n';
        continue;
      case 't':
        text[out++] = '\t';
        continue;
      case 'r':
        text[out++] = '\r';
        continue;
      case '\\':
        text[out++] = '\\';
        continue;
      case '"':
        text[out++] = '"';
        continue;
      case '\'':
        text[out++] = '\'';
        continue;
      case 'x': {
        /* \xNN — byte hex, 1–2 digit (design/next_print.txt color:
         * fondasi ANSI manual seperti "\x1b[31m" sebelum U_RED). */
        int hv = 0, nd = 0;
        while (nd < 2 && i + 1 < end && isxdigit((unsigned char)value[i + 1])) {
          char h = value[++i];
          int d = (h >= '0' && h <= '9')   ? h - '0'
                  : (h >= 'a' && h <= 'f') ? h - 'a' + 10
                                           : h - 'A' + 10;
          hv = (hv << 4) | d;
          nd++;
        }
        if (nd > 0) {
          text[out++] = (char)hv;
          continue;
        }
        text[out++] = value[i]; /* tanpa digit hex: 'x' apa adanya */
        continue;
      }
      default:
        text[out++] = value[i];
        continue;
      }
    }
    text[out++] = value[i];
  }
  text[out] = '\0';
  return (RuntimeValue){.type = VALUE_STRING, .as.string = text};
}

RuntimeValue valueArray(RuntimeValue *items, int length) {
  return (RuntimeValue){.type = VALUE_ARRAY, .as.array = {.items = items, .length = length}};
}

/**
 * Evaluate a small Rupa expression snippet (e.g. "add(1,2)" or
 * "users.name") extracted from inside a `{{ ... }}` interpolation block.
 *
 * Uses the CURRENT runtime environment `env` so it can see already
 * declared variables and functions - it builds a throwaway token/AST pool
 * for just this snippet, but that's fine: RuntimeFunction carries its own
 * defining Node pool (see interpretCall()), and variable/function lookup
 * goes through `env` by name, not through which Node pool is asking.
 *
 * Returns true and fills *out on success. Returns false (leaving *out
 * untouched) on lex/parse/eval failure, so the caller can fall back to
 * printing the interpolation block literally instead of crashing.
 */
static bool evalInterpExpr(const char *exprSrc, RuntimeEnv *env, Error *error, RuntimeValue *out) {
  if (!exprSrc || !*exprSrc) return false;

  State *state = createGlobalState(8, false);
  if (!state || !state->buffer) return false;
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;

  Buffer *buffer = state->buffer;
  size_t len = strlen(exprSrc);
  if ((int)len >= buffer->capacity) return false;

  memcpy(buffer->value, exprSrc, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  if (!addToHistory(state) || !addToInput(state)) return false;

  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) return false;

  Request request = createRequest(tokens, 8);
  int end = grammarLineEnd(tokens, 0);
  int exprId = grammarParseExpr(&request, 0, end);
  if (exprId < 0) {
    if (error)
      addError(error, (ErrorInfo){.code = "SyntaxError",
                                  .message = "invalid expression inside {{ }}",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_SYNTAX});
    return false;
  }

  InterpreterResult result = interpretNode(request.node, exprId, env, error);
  if (result.flow == FLOW_ERROR) return false;

  *out = result.value;
  return true;
}

/* ==================== Render-to-buffer (print engine) ====================
 * Satu pintu render untuk print engine (print_format.c): pola 1
 * (interpolasi), pola 2 (multi-arg), dan pola 4 (stream) semuanya
 * merender ke buffer — output akhir ditulis printRenderArgs().
 * printBufAppend = bufAppend versi publik (lihat expression/binary.c). */

void printBufAppend(char **buf, size_t *len, size_t *cap, const char *text) {
  if (!text) return;
  size_t tlen = strlen(text);
  while (*len + tlen + 1 > *cap) {
    *cap = (*cap) ? (*cap) * 2 : 128;
    char *grown = realloc(*buf, *cap);
    if (!grown) return; /* buffer lama tetap valid (NULL-terminated) */
    *buf = grown;
  }
  memcpy(*buf + *len, text, tlen);
  *len += tlen;
  (*buf)[*len] = '\0';
}

void valuePrintTo(char **buf, size_t *len, size_t *cap, RuntimeValue value) {
  char piece[64];
  switch (value.type) {
  case VALUE_NUMBER:
    snprintf(piece, sizeof(piece), "%lld", value.as.number);
    printBufAppend(buf, len, cap, piece);
    break;
  case VALUE_DECIMAL:
    snprintf(piece, sizeof(piece), "%g", value.as.decimal);
    printBufAppend(buf, len, cap, piece);
    break;
  case VALUE_BOOLEAN:
    printBufAppend(buf, len, cap, value.as.boolean ? "true" : "false");
    break;
  case VALUE_STRING:
    printBufAppend(buf, len, cap, value.as.string ? value.as.string : "");
    break;
  case VALUE_FUNCTION:
    printBufAppend(buf, len, cap, "<function>");
    break;
  case VALUE_NATIVE_FUNCTION:
    printBufAppend(buf, len, cap, "<function>");
    break;
  case VALUE_PTR:
    printBufAppend(buf, len, cap, value.as.ptr ? "<ptr>" : "null");
    break;
  case VALUE_ARRAY:
    printBufAppend(buf, len, cap, "[");
    for (int i = 0; i < value.as.array.length; i++) {
      if (i) printBufAppend(buf, len, cap, ", ");
      valuePrintTo(buf, len, cap, value.as.array.items[i]);
    }
    printBufAppend(buf, len, cap, "]");
    break;
  case VALUE_OBJECT: {
    bool first = true;
    printBufAppend(buf, len, cap, "{");
    for (struct RuntimeObjectEntry *e = value.as.object.entries; e; e = e->next) {
      /* Skip hidden metadata: sejajar valuePrint(). */
      if (e->key && (e->key[0] == '\0' || !strncmp(e->key, "__", 2) ||
                     strcmp(e->key, "_private") == 0 ||
                     strcmp(e->key, "_imports") == 0 ||
                     strcmp(e->key, "_class") == 0 ||
                     strcmp(e->key, "_created") == 0))
        continue;
      if (!first) printBufAppend(buf, len, cap, ", ");
      printBufAppend(buf, len, cap, e->key ? e->key : "?");
      printBufAppend(buf, len, cap, ": ");
      valuePrintTo(buf, len, cap, e->value);
      first = false;
    }
    printBufAppend(buf, len, cap, "}");
    break;
  }
  default:
    printBufAppend(buf, len, cap, "undefined");
    break;
  }
}

/* printStringWithInterp versi buffer — byte-for-byte sama dengan jalur
 * putchar di bawah (dipakai print engine pola 1/4). */
static void printStringInterpBuf(char **buf, size_t *len, size_t *cap, const char *str,
                                 RuntimeEnv *env, Error *error) {
  if (!str) return;
  const char *p = str;
  while (*p) {
    /* `{{ expr }}`: full expression - function call, member access, dst. */
    if (p[0] == '{' && p[1] == '{') {
      const char *end = strstr(p + 2, "}}");
      if (end) {
        int ilen = (int)(end - p - 2);
        if (ilen > 0) {
          char *expr = malloc((size_t)ilen + 1);
          if (!expr) {
            p = end + 2;
            continue;
          }
          memcpy(expr, p + 2, (size_t)ilen);
          expr[ilen] = '\0';

          RuntimeValue val;
          if (evalInterpExpr(expr, env, error, &val)) {
            valuePrintTo(buf, len, cap, val);
          } else {
            printBufAppend(buf, len, cap, "{{");
            printBufAppend(buf, len, cap, expr);
            printBufAppend(buf, len, cap, "}}");
          }
          free(expr);
        } else {
          printBufAppend(buf, len, cap, "{{}}"); /* empty braces */
        }
        p = end + 2;
        continue;
      }
    }
    /* `{ name }`: nama variabel tunggal (lookup langsung, tanpa parser). */
    if (*p == '{') {
      const char *end = strchr(p + 1, '}');
      if (end) {
        int ilen = (int)(end - p - 1);
        if (ilen > 0) {
          char *name = malloc((size_t)ilen + 1);
          if (!name) {
            p = end + 1;
            continue;
          }
          memcpy(name, p + 1, (size_t)ilen);
          name[ilen] = '\0';
          RuntimeValue val;
          if (env && semGet(env, name, &val)) {
            valuePrintTo(buf, len, cap, val);
          } else if (evalInterpExpr(name, env, error, &val)) {
            /* Bukan variable: coba sebagai expression (design pola 1
             * `print("{true}")` — literal/keyword bukan binding env). */
            valuePrintTo(buf, len, cap, val);
          } else {
            printBufAppend(buf, len, cap, "{");
            printBufAppend(buf, len, cap, name);
            printBufAppend(buf, len, cap, "}");
          }
          free(name);
        } else {
          printBufAppend(buf, len, cap, "{}"); /* empty braces */
        }
        p = end + 1;
        continue;
      }
    }
    char c[2] = {*p, 0};
    printBufAppend(buf, len, cap, c);
    p++;
  }
}

static void printStringWithInterp(const char *str, RuntimeEnv *env, Error *error) {
  if (!str) return;
  char *buf = NULL;
  size_t len = 0, cap = 0;
  printStringInterpBuf(&buf, &len, &cap, str, env, error);
  if (buf) {
    fwrite(buf, 1, len, stdout);
    free(buf);
  }
}

void valuePrintInterp(RuntimeValue value, RuntimeEnv *env, Error *error) {
  /* Provenance .spec (design rupa_go): object hasil `import spec from
   * rupa` bertag __spec — print ditolak mentah-mentah. Tag menempel di
   * VALUE (disalin by value), jadi alias (specx = spec) ikut tertolak;
   * variable biasa tanpa tag tetap bebas. Leaf scalar bebas. */
  if (value.type == VALUE_OBJECT) {
    RuntimeValue tag = valueNull();
    if (valueObjectGet(value, "__spec", &tag) && tag.type == VALUE_BOOLEAN &&
        tag.as.boolean) {
      if (error)
        addError(error, (ErrorInfo){.code = (char *)"SpecError",
                                    .message = (char *)"print(spec) ditolak — "
                                               "konfigurasi .spec tidak boleh "
                                               "ditampilkan; akses field-nya "
                                               "(spec.settings.host)",
                                    .line = 0,
                                    .row = 0,
                                    .type = ERR_TYPE_MISMATCH});
      return;
    }
  }
  if (value.type == VALUE_STRING) {
    printStringWithInterp(value.as.string, env, error);
    return;
  }
  valuePrint(value);
}

/* Provenance .spec versi engine (print_format.c): return true + error
 * SpecError bila value object bertag __spec — print engine menolak
 * merender-nya ke stream mana pun. Sejajar valuePrintInterp(). */
bool valueSpecRejected(RuntimeValue value, Error *error) {
  if (value.type != VALUE_OBJECT) return false;
  RuntimeValue tag = valueNull();
  if (valueObjectGet(value, "__spec", &tag) && tag.type == VALUE_BOOLEAN &&
      tag.as.boolean) {
    if (error)
      addError(error, (ErrorInfo){.code = (char *)"SpecError",
                                  .message = (char *)"print(spec) ditolak — "
                                             "konfigurasi .spec tidak boleh "
                                             "ditampilkan; akses field-nya "
                                             "(spec.settings.host)",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_TYPE_MISMATCH});
    return true;
  }
  return false;
}

/* Interpolasi versi engine (pola 1 di print_format.c): wrapper ke
 * printStringInterpBuf. NULL buffer = render langsung ke stdout. */
void printStringInterpTo(char **buf, size_t *len, size_t *cap, const char *str,
                         RuntimeEnv *env, Error *error) {
  if (!buf) {
    printStringWithInterp(str, env, error);
    return;
  }
  printStringInterpBuf(buf, len, cap, str, env, error);
}

void valuePrint(RuntimeValue value) {
  switch (value.type) {
  case VALUE_NUMBER:
    printf("%lld", value.as.number);
    break;
  case VALUE_DECIMAL:
    printf("%g", value.as.decimal);
    break;
  case VALUE_BOOLEAN:
    printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VALUE_STRING:
    printf("%s", value.as.string ? value.as.string : "");
    break;
  case VALUE_FUNCTION:
    printf("<function>");
    break;
  case VALUE_PTR:
    printf(value.as.ptr ? "<ptr>" : "null");
    break;
  case VALUE_ARRAY:
    putchar('[');
    for (int i = 0; i < value.as.array.length; i++) {
      if (i) printf(", ");
      valuePrint(value.as.array.items[i]);
    }
    putchar(']');
    break;
  case VALUE_OBJECT: {
    bool first = true;
    putchar('{');
    for (struct RuntimeObjectEntry *e = value.as.object.entries; e; e = e->next) {
      /* Skip hidden metadata: anchor implisit + meta "__*" (instance
       * new Object: __object/__type/__ref/__refname) + registry lama +
       * _imports (arsip binding hasil import, tree export). */
      if (e->key && (e->key[0] == '\0' || !strncmp(e->key, "__", 2) ||
                     strcmp(e->key, "_private") == 0 ||
                     strcmp(e->key, "_imports") == 0 ||
                     strcmp(e->key, "_class") == 0 ||
                     strcmp(e->key, "_created") == 0))
        continue;
      if (!first) printf(", ");
      printf("%s: ", e->key ? e->key : "?");
      valuePrint(e->value);
      first = false;
    }
    putchar('}');
    break;
  }
  default:
    printf("undefined");
    break;
  }
}

bool valueTruthy(RuntimeValue value) {
  switch (value.type) {
  case VALUE_NULL:
    return false;
  case VALUE_BOOLEAN:
    return value.as.boolean;
  case VALUE_NUMBER:
    return value.as.number != 0;
  case VALUE_DECIMAL:
    return value.as.decimal != 0;
  case VALUE_STRING:
    return value.as.string && value.as.string[0];
  default:
    return true;
  }
}

bool valueEquals(RuntimeValue left, RuntimeValue right) {
  if (left.type != right.type) return false;
  switch (left.type) {
  case VALUE_NULL:
    return true;
  case VALUE_BOOLEAN:
    return left.as.boolean == right.as.boolean;
  case VALUE_NUMBER:
    return left.as.number == right.as.number;
  case VALUE_DECIMAL:
    return left.as.decimal == right.as.decimal;
  case VALUE_STRING:
    return left.as.string && right.as.string && !strcmp(left.as.string, right.as.string);
  case VALUE_ARRAY: {
    /* Structural (deep) equality, element-wise. */
    if (left.as.array.length != right.as.array.length) return false;
    for (int i = 0; i < left.as.array.length; i++)
      if (!valueEquals(left.as.array.items[i], right.as.array.items[i])) return false;
    return true;
  }
  case VALUE_OBJECT: {
    /* Structural equality: same key set with equal values. */
    int leftCount = 0, rightCount = 0;
    for (struct RuntimeObjectEntry *e = left.as.object.entries; e; e = e->next)
      leftCount++;
    for (struct RuntimeObjectEntry *e = right.as.object.entries; e; e = e->next)
      rightCount++;
    if (leftCount != rightCount) return false;
    for (struct RuntimeObjectEntry *e = left.as.object.entries; e; e = e->next) {
      RuntimeValue rv;
      if (!valueObjectGet(right, e->key, &rv)) return false;
      if (!valueEquals(e->value, rv)) return false;
    }
    return true;
  }
  case VALUE_FUNCTION:
    return left.as.function == right.as.function;
  case VALUE_NATIVE_FUNCTION:
    return left.as.nativeFunc == right.as.nativeFunc;
  case VALUE_PTR:
    return left.as.ptr == right.as.ptr;
  default:
    return false;
  }
}

RuntimeValue valueFunction(RuntimeFunction *function) {
  return (RuntimeValue){.type = VALUE_FUNCTION, .as.function = function};
}

RuntimeValue valueObject(struct RuntimeObjectEntry *entries) {
  return (RuntimeValue){.type = VALUE_OBJECT, .as.object = {.entries = entries}};
}

RuntimeValue valueNativeFunction(const char *name, NativeFn func, int paramCount) {
  struct RuntimeNativeFunction *nf = gccalloc(1, sizeof(*nf));
  if (!nf) return valueNull();
  nf->name = name;
  nf->func = func;
  nf->paramCount = paramCount;
  return (RuntimeValue){.type = VALUE_NATIVE_FUNCTION, .as.nativeFunc = nf};
}

RuntimeValue valuePtr(void *ptr) {
  return (RuntimeValue){.type = VALUE_PTR, .as.ptr = ptr};
}

/* Color value (design/next_print.txt Color print): dibungkus VALUE_OBJECT
 * bertag "__color" — sejajar "__spec" (provenance .spec), bukan ValueType
 * baru supaya tidak menyentuh setiap switch(value.type) yang sudah ada.
 * "__rgb" membawa 24-bit RGB (0xRRGGBB) sebagai VALUE_NUMBER. Keduanya
 * berawalan "__" sehingga otomatis tersembunyi dari print objek biasa,
 * equality, dan salinan modul (lihat valuePrint/valueEquals). */
RuntimeValue valueColor(long long rgb) {
  struct RuntimeObjectEntry *tag = gccalloc(1, sizeof(*tag));
  struct RuntimeObjectEntry *val = gccalloc(1, sizeof(*val));
  if (!tag || !val) return valueNull();
  tag->key = gcstrdup("__color");
  tag->value = valueBoolean(true);
  val->key = gcstrdup("__rgb");
  val->value = valueNumber(rgb);
  tag->next = val;
  val->next = NULL;
  return valueObject(tag);
}

/* true bila value adalah color (valueColor) — *rgbOut diisi RGB-nya.
 * Dipakai print engine (print_format.c) untuk membungkus output dengan
 * escape ANSI truecolor, sejajar valueSpecRejected() untuk __spec. */
bool valueColorOf(RuntimeValue value, long long *rgbOut) {
  if (value.type != VALUE_OBJECT) return false;
  RuntimeValue tag = valueNull();
  if (!valueObjectGet(value, "__color", &tag) || tag.type != VALUE_BOOLEAN ||
      !tag.as.boolean)
    return false;
  RuntimeValue rgb = valueNull();
  if (rgbOut) *rgbOut = valueObjectGet(value, "__rgb", &rgb) && rgb.type == VALUE_NUMBER
                            ? rgb.as.number
                            : 0;
  return true;
}

bool valueObjectGet(RuntimeValue obj, const char *key, RuntimeValue *out) {
  if (obj.type != VALUE_OBJECT || !key) return false;
  for (struct RuntimeObjectEntry *e = obj.as.object.entries; e; e = e->next)
    if (e->key && !strcmp(e->key, key)) {
      if (out) *out = e->value;
      return true;
    }
  return false;
}

bool valueObjectSet(RuntimeValue *obj, const char *key, RuntimeValue value) {
  if (!obj || obj->type != VALUE_OBJECT || !key) return false;
  /* Update existing entry */
  for (struct RuntimeObjectEntry *e = obj->as.object.entries; e; e = e->next)
    if (e->key && !strcmp(e->key, key)) {
      e->value = value;
      return true;
    }
  /* Add new entry — APPEND, bukan prepend: RuntimeValue disalin by value
   * di banyak titik (call frame, this binding, member assign), jadi
   * mengganti head list hanya terlihat di salinan. Append menyambung
   * node baru ke TAIL list yang shared — mutasi terlihat di semua salinan
   * (mis. `this.value = v` dari dalam method harus persist ke instance).
   *
   * Object KOSONG (entries == NULL) mendapat anchor entry implisit "":
   * node pertama setelahnya disambung ke anchor yang DILIHAT SEMUA
   * SALINAN — receiver yang di-copy sebelum field pertama ditulis
   * (mis. `o = {}; o.set({a: 1})`, this binding) tetap melihat field.
   * Anchor disembunyikan dari print/equality. */
  if (!obj->as.object.entries) {
    struct RuntimeObjectEntry *anchor = gccalloc(1, sizeof(*anchor));
    if (!anchor) return false;
    anchor->key = gcstrdup("");
    anchor->value = valueNull();
    anchor->next = NULL;
    obj->as.object.entries = anchor;
  }
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  if (!e) return false;
  e->key = gcstrdup(key);
  e->value = value;
  e->next = NULL;
  struct RuntimeObjectEntry *tail = obj->as.object.entries;
  while (tail->next)
    tail = tail->next;
  tail->next = e;
  return true;
}

const char *valueTypeName(ValueType type) {
  switch (type) {
  case VALUE_NULL:
    return "null";
  case VALUE_NUMBER:
    return "number";
  case VALUE_DECIMAL:
    return "decimal";
  case VALUE_BOOLEAN:
    return "boolean";
  case VALUE_STRING:
    return "string";
  case VALUE_ARRAY:
    return "array";
  case VALUE_FUNCTION:
    return "function";
  case VALUE_OBJECT:
    return "object";
  case VALUE_NATIVE_FUNCTION:
    return "function";
  case VALUE_PTR:
    return "ptr";
  default:
    return "unknown";
  }
}
