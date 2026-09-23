#include <rupa.h>
#include "rpx.h"

/* Dynamic template .rpx (design/rupa_rpx.txt):
 * - .rpx = HTML murni dengan marker id="@name" (res/view entry).
 * - resources membaca file .rpx, mengumpulkan marker, dan memvalidasi
 *   tag terlarang (<style>, <head>, <script>, <noscript>, <link>, <body>).
 * - render(template, content) menyuntik content ke setiap marker.
 * - Akses marker: r.id.card → __accessor hook di interpretMember
 *   (member.c) mengevaluasi hook secara eager → object handle
 *   {"@marker": name} → render(handle, "Hello") menghasilkan markup. */

/* Tag terlarang dalam .rpx (Batasan dinamis template rpx). */
static const char *rpxForbiddenTags[] = {
    "style", "head", "script", "noscript", "link", "body",
    "STYLE", "HEAD", "SCRIPT", "NOSCRIPT", "LINK", "BODY",
    NULL};

/* ==================== Pemindaian template ==================== */

/* Hasil pemindaian: urutan kemunculan marker dipertahankan. */
typedef struct {
  char **names; /* nama marker (tanpa '@'), urut kemunculan */
  int count;
  bool has_forbidden;
  char forbidden_tag[32]; /* tag terlarang pertama yang ditemukan */
} RpxScan;

static void rpxScanInit(RpxScan *s) {
  s->names = NULL;
  s->count = 0;
  s->has_forbidden = false;
  s->forbidden_tag[0] = '\0';
}

static void rpxScanFree(RpxScan *s) {
  if (!s->names) {
    s->count = 0;
    return;
  }
  for (int i = 0; i < s->count; i++)
    free(s->names[i]);
  free(s->names);
  s->names = NULL;
  s->count = 0;
}

/* Tag terlarang? Pencocokan case-sensitive persis daftar di atas. */
static const char *rpxForbiddenMatch(const char *tag, size_t len) {
  for (int i = 0; rpxForbiddenTags[i]; i++) {
    size_t flen = strlen(rpxForbiddenTags[i]);
    if (flen == len && strncmp(tag, rpxForbiddenTags[i], len) == 0)
      return rpxForbiddenTags[i];
  }
  return NULL;
}

/* Pindai konten: kumpulkan marker id="@name" dan deteksi tag terlarang. */
static bool rpxScan(const char *html, RpxScan *s) {
  size_t cap = 0;
  size_t len = strlen(html);
  size_t i = 0;

  while (i < len) {
    if (html[i] != '<') {
      i++;
      continue;
    }

    /* Komentar <!-- ... --> dilewati utuh. */
    if (strncmp(html + i, "<!--", 4) == 0) {
      const char *end = strstr(html + i + 4, "-->");
      i = end ? (size_t)(end - html) + 3 : len;
      continue;
    }

    i++; /* lewati '<' */
    if (!isalpha((unsigned char)html[i])) continue; /* bukan tag: <5, </div */

    /* Nama tag: [A-Za-z][A-Za-z0-9-]* */
    size_t tstart = i;
    while (i < len && (isalnum((unsigned char)html[i]) || html[i] == '-'))
      i++;
    size_t tlen = i - tstart;

    const char *forbidden = rpxForbiddenMatch(html + tstart, tlen);
    if (forbidden && !s->has_forbidden) {
      s->has_forbidden = true;
      snprintf(s->forbidden_tag, sizeof(s->forbidden_tag), "%s", forbidden);
    }

    /* Atribut dalam tag sampai '>'. */
    while (i < len && html[i] != '>') {
      if (html[i] == 'i' && html[i + 1] == 'd' &&
          (html[i + 2] == '=' || html[i + 2] == ' ' || html[i + 2] == '\t')) {
        const char *p = html + i + 2;
        while (*p == ' ' || *p == '\t')
          p++;
        if (*p == '=') {
          p++;
          while (*p == ' ' || *p == '\t')
            p++;
          if (*p == '"' || *p == '\'') {
            char quote = *p++;
            const char *close = strchr(p, quote);
            size_t vlen = close ? (size_t)(close - p) : strlen(p);
            /* Marker: nilai atribut diawali '@'. */
            if (vlen >= 2 && p[0] == '@') {
              if ((size_t)s->count >= cap) {
                cap = cap ? cap * 2 : 8;
                char **tmp = realloc(s->names, cap * sizeof(char *));
                if (!tmp) {
                  rpxScanFree(s);
                  return false;
                }
                s->names = tmp;
              }
              char *name = malloc(vlen); /* '@' diganti '\0' */
              if (!name) {
                rpxScanFree(s);
                return false;
              }
              memcpy(name, p + 1, vlen - 1);
              name[vlen - 1] = '\0';
              s->names[s->count++] = name;
            }
            if (close)
              i = (size_t)(close - html) + 1;
            else
              i = len;
            continue;
          }
        }
      }
      i++;
    }
    if (i < len) i++; /* lewati '>' */
  }
  return true;
}

/* Helper error modul. */
static InterpreterResult rpxError(Error *error, const char *code,
                                  const char *msg) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)code,
                                .message = (char *)msg,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  return resultFlow(FLOW_ERROR, valueNull());
}

/* Baca + scan file .rpx. Return false bila file tak terbaca / ditolak.
 * Error sudah dicatat sebelum return false. */
static bool rpxLoadScan(const char *path, char **out_html, RpxScan *scan,
                        Error *error, const char *who) {
  *out_html = NULL;
  rpxScanInit(scan);

  if (!path || !*path) {
    static char msg[160];
    snprintf(msg, sizeof(msg), "%s() expects a .rpx template path", who);
    rpxError(error, "TypeError", msg);
    return false;
  }

  /* Enforce ekstensi .rpx — template dinamis hanya lewat file ini. */
  size_t plen = strlen(path);
  if (plen < 5 || strcmp(path + plen - 4, ".rpx") != 0) {
    static char msg[160];
    snprintf(msg, sizeof(msg), "%s() expects a .rpx template file, got '%s'",
             who, path);
    rpxError(error, "TemplateError", msg);
    return false;
  }

  InterpreterResult fr = fsbaseReadString(path, error);
  if (fr.flow != FLOW_NORMAL || fr.value.type != VALUE_STRING ||
      !fr.value.as.string)
    return false; /* fsbaseReadString sudah mencatat IOError */

  char *html = fr.value.as.string; /* GC-managed (valueString) — jangan free */
  if (!rpxScan(html, scan)) {
    rpxError(error, "IOError", "out of memory reading template");
    return false;
  }

  if (scan->has_forbidden) {
    static char msg[192];
    snprintf(msg, sizeof(msg), "template '%s' uses forbidden tag <%s>", path,
             scan->forbidden_tag);
    rpxError(error, "TemplateError", msg);
    rpxScanFree(scan);
    return false;
  }

  *out_html = html;
  return true;
}

/* ==================== resources.read(path) ==================== */
/* Return isi template apa adanya (string), setelah validasi. */
static InterpreterResult rpxRead(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return rpxError(error, "TypeError",
                    "resources.read() expects a string path");

  char *html = NULL;
  RpxScan scan;
  if (!rpxLoadScan(argv[0].as.string, &html, &scan, error, "resources.read"))
    return resultFlow(FLOW_ERROR, valueNull());

  RuntimeValue out = valueString(html); /* copy — html GC-managed */
  rpxScanFree(&scan);
  return resultNormal(out);
}

/* ==================== resources.ids(path) ==================== */
/* Return array nama marker (urut kemunculan, tanpa '@'). */
static InterpreterResult rpxIds(int argc, RuntimeValue *argv,
                                RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return rpxError(error, "TypeError",
                    "resources.ids() expects a string path");

  char *html = NULL;
  RpxScan scan;
  if (!rpxLoadScan(argv[0].as.string, &html, &scan, error, "resources.ids"))
    return resultFlow(FLOW_ERROR, valueNull());

  RuntimeValue *items =
      scan.count > 0 ? gcmall(sizeof(RuntimeValue) * (size_t)scan.count) : NULL;
  if (scan.count > 0 && !items) {
    rpxScanFree(&scan);
    return rpxError(error, "IOError", "out of memory");
  }
  for (int i = 0; i < scan.count; i++)
    items[i] = valueString(scan.names[i]);

  RuntimeValue out = valueArray(items, scan.count);
  rpxScanFree(&scan);
  return resultNormal(out);
}

/* ==================== resources.source(path) ==================== */
/* Return object { html, ids } — html mentah, ids array nama marker. */
static InterpreterResult rpxSource(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return rpxError(error, "TypeError",
                    "resources.source() expects a string path");

  char *html = NULL;
  RpxScan scan;
  if (!rpxLoadScan(argv[0].as.string, &html, &scan, error, "resources.source"))
    return resultFlow(FLOW_ERROR, valueNull());

  struct RuntimeObjectEntry *entries = NULL;

  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  e->key = gcstrdup("html");
  e->value = valueString(html);
  e->next = entries;
  entries = e;

  e = gccalloc(1, sizeof(*e));
  e->key = gcstrdup("ids");
  RuntimeValue *items =
      scan.count > 0 ? gcmall(sizeof(RuntimeValue) * (size_t)scan.count) : NULL;
  for (int i = 0; i < scan.count; i++)
    items[i] = valueString(scan.names[i]);
  e->value = valueArray(items, scan.count);
  e->next = entries;
  entries = e;

  rpxScanFree(&scan);
  return resultNormal(valueObject(entries));
}

/* ==================== Hook __accessor ==================== */
/* Handler hook di interpretMember: dipanggil EAGER untuk member tak dikenal
 * pada object resources. Argumen: (receiver, memberName).
 *   memberName == "id" → namespace marker: return object yang SATU
 *     entrynya "__accessor" menunjuk rpxMarkerAccess, sehingga rantai
 *     berikutnya (r.id.card) di-resolve oleh hook yang sama.
 *   Selain itu → object handle {"@marker": name} untuk render(). */
InterpreterResult rpxResourceAccess(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || argv[1].type != VALUE_STRING || !argv[1].as.string)
    return resultNormal(valueNull());

  const char *member = argv[1].as.string;

  if (strcmp(member, "id") == 0) {
    /* Sub-namespace marker: satu-satunya entry adalah hook lanjutan. */
    struct RuntimeObjectEntry *acc = gccalloc(1, sizeof(*acc));
    acc->key = gcstrdup("__accessor");
    acc->value = valueNativeFunction("__accessor", rpxResourceAccess, 2);
    return resultNormal(valueObject(acc));
  }

  /* Marker biasa → handle render. */
  struct RuntimeObjectEntry *entries = NULL;
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  e->key = gcstrdup("@marker");
  e->value = valueString(member);
  e->next = entries;
  entries = e;
  return resultNormal(valueObject(entries));
}

/* ==================== resources.has(path, name) ==================== */
/* Pemakaian langsung: verifikasi marker ada di template. */
static InterpreterResult rpxIdCheck(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || argv[0].type != VALUE_STRING || !argv[0].as.string ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return rpxError(error, "TypeError",
                    "resources.has() expects (path, name) strings");

  char *html = NULL;
  RpxScan scan;
  if (!rpxLoadScan(argv[0].as.string, &html, &scan, error, "resources.has"))
    return resultFlow(FLOW_ERROR, valueNull());

  bool found = false;
  for (int i = 0; i < scan.count; i++) {
    if (strcmp(scan.names[i], argv[1].as.string) == 0) {
      found = true;
      break;
    }
  }
  rpxScanFree(&scan);

  if (!found) {
    static char msg[224];
    snprintf(msg, sizeof(msg), "marker '@%s' not found in template '%s'",
             argv[1].as.string, argv[0].as.string);
    rpxError(error, "TemplateError", msg);
    return resultFlow(FLOW_ERROR, valueNull());
  }
  return resultNormal(valueBoolean(true));
}

/* ==================== render(target, content?) ==================== */
/* Dua bentuk (design/rupa_rpx.txt):
 *   render(card, "Hello")  — card dari r.id.card: markup element dengan
 *                            content di dalam marker.
 *   render("x.rpx", "Hi")  — inject ke SEMUA marker template.
 *   render(t) / render(t, null) — marker dikosongkan. */
static InterpreterResult rpxRender(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1)
    return rpxError(error, "TypeError",
                    "render() expects a template path or r.id.<name> handle");

  const char *content = "";
  if (argc >= 2 && argv[1].type == VALUE_STRING && argv[1].as.string)
    content = argv[1].as.string;
  else if (argc >= 2 && argv[1].type != VALUE_NULL)
    return rpxError(error, "TypeError", "render() expects string content");

  /* --- Bentuk 1: handle marker dari r.id.<name> --- */
  if (argv[0].type == VALUE_OBJECT) {
    RuntimeValue marker = valueNull();
    if (!valueObjectGet(argv[0], "@marker", &marker) ||
        marker.type != VALUE_STRING || !marker.as.string)
      return rpxError(error, "TypeError",
                      "render() expects a template path or r.id.<name> handle");

    /* Element markup sederhana: content dibungkus span marker.
     * Template engine final (rupa go web) mengganti sesuai tag asli;
     * bentuk ini menjaga kontrak render(card, "Hello") tetap usable
     * dari CLI/interpreter biasa. */
    size_t need = strlen(marker.as.string) + strlen(content) + 64;
    char *buf = malloc(need);
    if (!buf)
      return rpxError(error, "IOError", "out of memory");
    snprintf(buf, need, "<span data-marker=\"%s\">%s</span>",
             marker.as.string, content);
    RuntimeValue out = valueString(buf);
    free(buf);
    return resultNormal(out);
  }

  /* --- Bentuk 2: path template .rpx --- */
  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return rpxError(error, "TypeError",
                    "render() expects a template path or r.id.<name> handle");

  char *html = NULL;
  RpxScan scan;
  if (!rpxLoadScan(argv[0].as.string, &html, &scan, error, "render"))
    return resultFlow(FLOW_ERROR, valueNull());

  /* String buffer output dengan growth doubling. */
  size_t cap = strlen(html) + strlen(content) * ((size_t)scan.count + 1) + 256;
  char *out = malloc(cap);
  if (!out) {
    rpxScanFree(&scan);
    return rpxError(error, "IOError", "out of memory");
  }
  size_t olen = 0;
  size_t pos = 0;
  size_t len = strlen(html);
  int markerIdx = 0; /* urutan marker = urutan kemunculan */

#define RPX_PUT(str, slen)                                                \
  do {                                                                    \
    if (olen + (size_t)(slen) + 1 > cap) {                                \
      cap = (olen + (size_t)(slen) + 1) * 2;                              \
      char *tmp = realloc(out, cap);                                      \
      if (!tmp) {                                                         \
        free(out);                                                        \
        rpxScanFree(&scan);                                               \
        return rpxError(error, "IOError", "out of memory");               \
      }                                                                   \
      out = tmp;                                                          \
    }                                                                     \
    memcpy(out + olen, (str), (size_t)(slen));                            \
    olen += (size_t)(slen);                                               \
    out[olen] = '\0';                                                     \
  } while (0)

  while (pos < len) {
    if (markerIdx >= scan.count) {
      /* Marker habis — salin sisanya. */
      RPX_PUT(html + pos, len - pos);
      break;
    }

    /* Cari atribut marker berikutnya: id="@name" (atau single quote). */
    char needle[512];
    int printed =
        snprintf(needle, sizeof(needle), "id=\"@%s\"", scan.names[markerIdx]);
    if (printed < 0 || (size_t)printed >= sizeof(needle)) {
      markerIdx++;
      continue;
    }
    const char *hit = strstr(html + pos, needle);
    if (!hit) {
      printed = snprintf(needle, sizeof(needle), "id='@%s'",
                         scan.names[markerIdx]);
      if (printed > 0 && (size_t)printed < sizeof(needle))
        hit = strstr(html + pos, needle);
    }
    if (!hit) {
      markerIdx++; /* marker tak muncul lagi — lanjut marker berikutnya */
      continue;
    }
    size_t attrStart = (size_t)(hit - html);

    /* Cari '>' penutup tag pembuka SEBELUM menyalin, supaya posisi aman. */
    const char *gt = strchr(html + attrStart, '>');
    if (!gt) {
      /* Malformed — salin sisanya apa adanya. */
      RPX_PUT(html + pos, len - pos);
      break;
    }
    size_t tagEnd = (size_t)(gt - html) + 1;

    /* Salin sampai awal atribut marker. */
    RPX_PUT(html + pos, attrStart - pos);

    /* Lepas atribut id="@name" — potong [attrStart, tagEnd). */
    pos = tagEnd;

    /* Cari penutup elemen </...> pertama setelah tag pembuka — content
     * element diganti dengan inject. Tag terlarang sudah divalidasi. */
    const char *closeTag = strstr(html + pos, "</");
    size_t contentEnd = closeTag ? (size_t)(closeTag - html) : len;

    if (*content)
      RPX_PUT(content, strlen(content));

    pos = contentEnd;
    markerIdx++;
  }

#undef RPX_PUT

  RuntimeValue result = valueString(out);
  free(out); /* out malloc'd — valueString menyalin ke GC */
  rpxScanFree(&scan);
  return resultNormal(result);
}

/* ==================== Module init ==================== */
static void addEntry(struct RuntimeObjectEntry **head, const char *name,
                     NativeFn fn, int paramCount) {
  struct RuntimeObjectEntry *e = gccalloc(1, sizeof(*e));
  e->key = gcstrdup(name);
  e->value = valueNativeFunction(name, fn, paramCount);
  e->next = *head;
  *head = e;
}

InterpreterResult stdResourcesInit(Node *node, int id, RuntimeEnv *env,
                                   Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  struct RuntimeObjectEntry *entries = NULL;

  /* resources.read / ids / source — utilitas umum. Catatan: tidak ada
   * entry "id" — nama itu disediakan untuk namespace marker dinamis
   * r.id.<name> lewat hook __accessor (member lookup eksplisit lebih
   * dulu di interpretMember, jadi entry statis akan menutupi hook). */
  addEntry(&entries, "read", rpxRead, 1);
  addEntry(&entries, "ids", rpxIds, 1);
  addEntry(&entries, "source", rpxSource, 1);
  addEntry(&entries, "has", rpxIdCheck, 2);

  /* r.id.<name> → dynamic accessor (hook eager di interpretMember). */
  struct RuntimeObjectEntry *acc = gccalloc(1, sizeof(*acc));
  acc->key = gcstrdup("__accessor");
  acc->value = valueNativeFunction("__accessor", rpxResourceAccess, 2);
  acc->next = entries;
  entries = acc;

  return resultNormal(valueObject(entries));
}

InterpreterResult stdRenderInit(Node *node, int id, RuntimeEnv *env,
                                Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  struct RuntimeObjectEntry *entries = NULL;
  addEntry(&entries, "render", rpxRender, 2);
  return resultNormal(valueObject(entries));
}
