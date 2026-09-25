#include <rupa.h>

/**
 * @brief `rupa go [target]` — runner class berbasis proyek
 * (design/rupa_go.txt).
 *
 * Struktur proyek:
 *   /project
 *     /app/main.rp      <- entry: dicari otomatis
 *     /res/template.rpx
 *     /styles/style.rpx
 *     .spec             <- konfigurasi proyek
 *
 * Isi .spec (android):
 *   name: your app name
 *   author: -
 *   version: "1.0"
 *   target: android
 *   arch: unknown
 *   debug: false
 *   release: false
 *
 * Isi .spec (web):
 *   target: web
 *   debug: true
 *   release: false
 *   domain: example.com
 *   settings:
 *     - vps:
 *       - settingan vps
 *     - host: 127.0.0.1
 *     - port: 8000
 *     - protocol: http
 *
 * Pola konfigurasi ditiru dari formatter (format_config.c): walk-up dari
 * cwd sampai root mencari file konfigurasi, parser mini key:value,
 * struct config dengan defaults. `rupa go [target]` otomatis mencari
 * main.rp (app/main.rp, lalu main.rp). Argumen target boleh: satu kata
 * (`rupa go android`), daftar koma (`rupa go web,ios`), atau bracket
 * (`rupa go [web, ios]` — boleh terpecah beberapa token oleh shell).
 * Target di argumen menimpa target di .spec.
 * Compile/emisi per target (prioritas web) menyusul — kerangka ini
 * memvalidasi argumen + konfigurasi dan menjalankan pipeline penuh.
 */

#define GO_SPEC_MAX_VPS 8

typedef struct GoSettings {
  char *vps[GO_SPEC_MAX_VPS]; /* daftar item "- vps:" (design web) */
  int vpsCount;
  char *host;
  char *port;
  char *protocol;
  /* Database (kredensial — alasan utama enkripsi .spec): */
  char *dbhost;
  char *dbport;
  char *dbname;
  char *dbuser;
  char *dbpass;
} GoSettings;

typedef struct GoSpec {
  char *name;
  char *author;
  char *version;
  char *target;
  char *arch;
  bool debug;
  bool release;
  /* Web (design/rupa_go.txt bagian web_example): */
  char *domain;
  GoSettings settings;
  bool haveSettings;
  /* .spec terenkripsi ada tapi gagal dibuka (password salah/kosong):
   * eksekusi harus berhenti — jangan jalan dengan defaults. */
  bool encryptedFailed;
} GoSpec;

typedef struct GoOptions {
  const char *path;   /* entry file (explicit arg / main.rp hasil cari) */
  const char *target; /* dari argumen — menimpa .spec */
  const char *mode;   /* "dev" / "build" (design: Dev or build) */
  bool detail;        /* -d */
  bool haveTarget;
} GoOptions;

static const char *goTargets[] = {"android", "web", "software", "ios"};

static bool goTargetValid(const char *target) {
  for (size_t i = 0; i < sizeof(goTargets) / sizeof(goTargets[0]); i++)
    if (!strcmp(target, goTargets[i])) return true;
  return false;
}

/* ==================== .spec — konfigurasi proyek ==================== */
/* Pola fmtLoadConfig (format_config.c): walk-up dari cwd, parser mini
 * key:value, unknown key dibiarkan (forward compatibility). */

static char *goSpecTrim(char *s) {
  while (*s && isspace((unsigned char)*s)) s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1])) --end;
  *end = '\0';
  /* Buang pasangan kutip pembungkus: version: "1.0" */
  if (*s == '"' && end > s + 1 && end[-1] == '"') {
    *--end = '\0';
    s++;
  }
  return s;
}

static bool goSpecParseBool(const char *s, bool *out) {
  if (!s || !out) return false;
  if (!strcmp(s, "true") || !strcmp(s, "yes")) {
    *out = true;
    return true;
  }
  if (!strcmp(s, "false") || !strcmp(s, "no")) {
    *out = false;
    return true;
  }
  return false;
}

static void goSettingsAddVps(GoSpec *c, const char *value) {
  if (!c || !value || !*value) return;
  if (c->settings.vpsCount >= GO_SPEC_MAX_VPS) return;
  c->settings.vps[c->settings.vpsCount++] = gcstrdup(value);
}

static void goSpecApply(GoSpec *c, const char *section, const char *subsection,
                        const char *key, const char *value) {
  if (!c || !key || !value) return;

  /* Section settings (design web): host/port/protocol + list vps. */
  if (section && !strcmp(section, "settings")) {
    c->haveSettings = true;
    if (!subsection) {
      if (!strcmp(key, "host")) c->settings.host = gcstrdup(value);
      else if (!strcmp(key, "port")) c->settings.port = gcstrdup(value);
      else if (!strcmp(key, "protocol")) c->settings.protocol = gcstrdup(value);
      else if (!strcmp(key, "dbhost")) c->settings.dbhost = gcstrdup(value);
      else if (!strcmp(key, "dbport")) c->settings.dbport = gcstrdup(value);
      else if (!strcmp(key, "dbname")) c->settings.dbname = gcstrdup(value);
      else if (!strcmp(key, "dbuser")) c->settings.dbuser = gcstrdup(value);
      else if (!strcmp(key, "dbpass")) c->settings.dbpass = gcstrdup(value);
      else if (!strcmp(key, "vps"))
        goSettingsAddVps(c, value); /* "- vps: ssh@host" — nilai di header */
    } else if (!strcmp(subsection, "vps")) {
      goSettingsAddVps(c, value);
    }
    return;
  }

  /* Top-level (android + web). */
  if (!strcmp(key, "name")) c->name = gcstrdup(value);
  else if (!strcmp(key, "author")) c->author = gcstrdup(value);
  else if (!strcmp(key, "version")) c->version = gcstrdup(value);
  else if (!strcmp(key, "target")) c->target = gcstrdup(value);
  else if (!strcmp(key, "arch")) c->arch = gcstrdup(value);
  else if (!strcmp(key, "debug")) (void)goSpecParseBool(value, &c->debug);
  else if (!strcmp(key, "release")) (void)goSpecParseBool(value, &c->release);
  else if (!strcmp(key, "domain")) c->domain = gcstrdup(value);
}

static GoSpec goSpecDefaults(void) {
  GoSpec c = {0};
  c.name = gcstrdup("-");
  c.author = gcstrdup("-");
  c.version = gcstrdup("0.0.0");
  /* Prioritas target utama adalah web (design/rupa_go.txt). */
  c.target = gcstrdup("web");
  c.arch = gcstrdup("unknown");
  c.debug = false;
  c.release = false;
  c.domain = gcstrdup("-");
  c.settings.host = gcstrdup("127.0.0.1");
  c.settings.port = gcstrdup("8000");
  c.settings.protocol = gcstrdup("http");
  c.settings.dbhost = gcstrdup("127.0.0.1");
  c.settings.dbport = gcstrdup("5432");
  c.settings.dbname = gcstrdup("app");
  c.settings.dbuser = gcstrdup("-");
  c.settings.dbpass = gcstrdup("-");
  return c;
}

static bool goFileExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

/* Baca seluruh file ke buffer malloc — NULL bila gagal. */
static char *goReadAllFile(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }
  char *buf = malloc((size_t)size + 1);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  size_t got = fread(buf, 1, (size_t)size, fp);
  fclose(fp);
  buf[got] = '\0';
  return buf;
}

/* Parser .spec line-based (plaintext maupun hasil decrypt) — satu
 * implementasi untuk kedua jalur. */
static void goSpecParseText(GoSpec *c, const char *text) {
  char *copy = strdup(text);
  if (!copy) return;

  char *save = NULL;
  char section[64] = {0};
  char subsection[64] = {0};
  int subIndent = 0;
  for (char *lineTok = strtok_r(copy, "\n", &save); lineTok;
       lineTok = strtok_r(NULL, "\n", &save)) {
    char *raw = lineTok;
    char *comment = strchr(raw, '#');
    if (comment) *comment = '\0';
    char *t = goSpecTrim(raw);
    if (!*t) continue;
    int indent = 0;
    for (char *q = lineTok; *q && (*q == ' ' || *q == '\t'); q++)
      indent += (*q == '\t') ? 2 : 1;
    if (t[0] == '-') t = goSpecTrim(t + 1);
    if (!*t) continue;

    /* Sibling list item (indent kembali ke level header subsection)
     * keluar dari subsection: `- host:` setelah `- vps:` bukan anggota
     * vps — dia item settings berikutnya. */
    if (subsection[0] && indent <= subIndent) subsection[0] = '\0';

    char *colon = strchr(t, ':');
    if (!colon) {
      /* Item daftar tanpa key — design web: "- settingan vps". */
      if (!strcmp(section, "settings") && !strcmp(subsection, "vps"))
        goSettingsAddVps(c, t);
      continue;
    }
    *colon = '\0';
    char *key = goSpecTrim(t);
    char *value = goSpecTrim(colon + 1);

    if (!*value) {
      /* Header section (indent 0) / subsection (indent > 0). */
      if (indent == 0) {
        snprintf(section, sizeof(section), "%s", key);
        subsection[0] = '\0';
      } else if (section[0]) {
        snprintf(subsection, sizeof(subsection), "%s", key);
        subIndent = indent;
      }
      continue;
    }

    goSpecApply(c, section, subsection[0] ? subsection : NULL, key, value);
  }
  free(copy);
}

static GoSpec goSpecLoad(void) {
  GoSpec c = goSpecDefaults();
  char cwd[PATH_MAX];
  if (!getcwd(cwd, sizeof(cwd))) return c;

  /* Walk-up cari .spec (pola fmtLoadConfig). */
  char configPath[PATH_MAX + 16];
  char *dir = cwd;
  for (;;) {
    snprintf(configPath, sizeof(configPath), "%s/.spec", dir);
    if (goFileExists(configPath)) break;
    if (strcmp(dir, "/") == 0) return c;
    char *slash = strrchr(dir, '/');
    if (!slash) return c;
    if (slash == dir) {
      dir[1] = '\0';
    } else {
      *slash = '\0';
    }
  }

  char *content = goReadAllFile(configPath);
  if (!content) return c;

  /* .spec terenkripsi (rupa spec -e): header + hex. Prompt password,
   * decrypt penuh sebelum parse. Password salah = gagal jalan
   * (kredensial database tidak boleh dijawab dengan defaults). */
  if (!strncmp(content, "rupa-spec-enc:", 14)) {
    char algo[32] = {0};
    if (sscanf(content + 14, "%31s", algo) != 1) {
      fprintf(stderr, "rupa go: .spec terenkripsi tapi header algo tidak valid\n");
      c.encryptedFailed = true;
      free(content);
      return c;
    }
    const char *hex = strchr(content, '\n');
    if (!hex) {
      fprintf(stderr, "rupa go: .spec terenkripsi tidak berisi payload\n");
      c.encryptedFailed = true;
      free(content);
      return c;
    }
    hex++;
    /* Format: header algo, baris "check: <16hex>", lalu payload hex. */
    const char *checkHex = NULL;
    if (!strncmp(hex, "check: ", 7)) {
      checkHex = hex + 7;
      const char *nl = strchr(hex, '\n');
      if (!nl) {
        fprintf(stderr, "rupa go: .spec terenkripsi tidak berisi payload\n");
        c.encryptedFailed = true;
        free(content);
        return c;
      }
      hex = nl + 1;
    }
    char pass[256];
    if (!specPromptPassword("spec password: ", pass, sizeof(pass)) || !*pass) {
      fprintf(stderr, "rupa go: password kosong — .spec terenkripsi tidak dibuka\n");
      c.encryptedFailed = true;
      free(content);
      return c;
    }
    char *plain = specDecryptText(algo, pass, checkHex, hex);
    memset(pass, 0, sizeof(pass));
    free(content);
    if (!plain) {
      fprintf(stderr,
              "rupa go: decrypt gagal (password salah / .spec bukan output 'rupa spec -e')\n");
      c.encryptedFailed = true;
      return c;
    }
    goSpecParseText(&c, plain);
    free(plain);
    return c;
  }

  goSpecParseText(&c, content);
  free(content);
  return c;
}

/* ==================== entry point: auto-cari main.rp ==================== */

/* Explicit file menang; bila argumen bukan file, cari main.rp proyek:
 * app/main.rp (design), lalu main.rp di cwd. NULL bila tidak ketemu. */
static const char *goFindEntry(const char *explicitPath) {
  static char buf[PATH_MAX];

  if (explicitPath) {
    struct stat st;
    if (stat(explicitPath, &st) == 0 && S_ISREG(st.st_mode)) return explicitPath;
    return NULL;
  }

  static const char *candidates[] = {"app/main.rp", "main.rp"};
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    struct stat st;
    if (stat(candidates[i], &st) == 0 && S_ISREG(st.st_mode)) {
      snprintf(buf, sizeof(buf), "%s", candidates[i]);
      return buf;
    }
  }
  return NULL;
}

/* ==================== target list ==================== */

/* Tambah target ke opt (validasi); target pertama yang menang —
 * multi-target = emisi berkali-kali, menyusul setelah compile aktif. */
static bool goAddTarget(GoOptions *opt, const char *tok) {
  while (*tok == ' ') tok++;
  char *end = (char *)tok + strlen(tok);
  while (end > tok && end[-1] == ' ') end--;
  *end = '\0';
  if (!*tok) return true;
  if (!goTargetValid(tok)) {
    fprintf(stderr, "rupa go: unknown target '%s' (android, web, software, ios)\n", tok);
    return false;
  }
  if (!opt->haveTarget) {
    opt->target = gcstrdup(tok);
    opt->haveTarget = true;
  }
  return true;
}

/* Parse token target: "[web, ios]" bisa terpecah beberapa token oleh
 * shell. Return index token terakhir yang dikonsumsi, atau -1. */
static int goParseTargets(int argc, const char *argv[], int i, GoOptions *opt) {
  char list[256] = {0};
  int consumed = i;

  if (argv[i][0] == '[') {
    bool closed = false;
    while (consumed < argc && !closed) {
      const char *tok = argv[consumed];
      const char *open = list[0] ? NULL : strchr(tok, '[');
      const char *close = strchr(tok, ']');
      if (strlen(list) + strlen(tok) + 2 >= sizeof(list)) return -1;
      if (list[0]) strcat(list, " ");
      if (close) {
        if (open) {
          /* satu token: [web] / [web,ios] */
          strncat(list, open + 1, (size_t)(close - open - 1));
        } else {
          strncat(list, tok, (size_t)(close - tok));
        }
        closed = true; /* token penutup = consumed terakhir */
      } else if (open) {
        strcat(list, open + 1);
        consumed++;
      } else {
        strcat(list, tok);
        consumed++;
      }
    }
    if (!closed) return -1;
    for (int k = (int)strlen(list) - 1; k >= 0 && list[k] == ' '; k--)
      list[k] = '\0';
  } else {
    snprintf(list, sizeof(list), "%s", argv[i]);
    consumed = i;
  }

  char buf[128];
  snprintf(buf, sizeof(buf), "%s", list);
  char *save = NULL;
  for (char *tok = strtok_r(buf, ",", &save); tok;
       tok = strtok_r(NULL, ",", &save)) {
    if (!goAddTarget(opt, tok)) return -1;
  }
  return consumed;
}

/* ==================== jalankan ==================== */

/* Jalankan satu file via pipeline penuh — prinsip run() (runner.c):
 * lex -> AST -> IR -> executeIRError. construct() class dieksekusi via
 * trampoline IR_INTERP saat deklarasi class tereksekusi. */
static int goRunFile(const char *path, const GoOptions *opt) {
  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "rupa go: failed to create state\n");
    return 1;
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;
  analyzerReset();

  Buffer *buffer = state->buffer;
  if (!readfile(path, buffer)) {
    fprintf(stderr, "rupa go: cannot read file %s\n", path);
    return 1;
  }
  setSourceFilePath(path);

  addToHistory(state);
  addToInput(state);
  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
    if (opt->detail)
      fprintf(stderr, "rupa go: lexer stage failed (waiting=%d)\n",
              flags ? (int)flags->isWaiting : -1);
    printErrors(state->error);
    return 1;
  }

  Request request = createRequestWithError(tokens, 10, state->error);
  Node *node = processGenerate(&request);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    if (opt->detail) fprintf(stderr, "rupa go: parse stage failed\n");
    printErrors(state->error);
    return 1;
  }

  int root = -1;
  for (int j = 0; j < node->length; j++) {
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  }
  if (root < 0) {
    fprintf(stderr, "rupa go: program root not found\n");
    return 1;
  }

  IRModule *ir = createIR();
  if (!ir || !rewrite(node, -1, ir)) {
    if (opt->detail) fprintf(stderr, "rupa go: rewrite stage failed\n");
    printErrors(state->error);
    return 1;
  }

  Error *error = createError(10);
  int status = executeIRError(ir, node, error);
  if (status != 0) {
    printErrors(error);
    return status;
  }

  /* Modul single-shot — bebas dibebaskan setelah eksekusi. */
  irModuleFree(ir);
  return 0;
}

/* ==================== dev server (default) ==================== */

/* Server default mode dev — berpacu dari .spec.settings (design:
 * "sistem akan menjalankan server default yang berpacu dari
 * .spec.settings"). serve.rp milik user (bila ada) jalan dulu via
 * goRunFile; server default meng-hold proses di depan. Placeholder:
 * balasan HTTP 200 minimal sampai template .rpx aktif. */
static int goDevServer(const GoSpec *spec) {
#ifdef _WIN32
  (void)spec;
  fprintf(stderr, "rupa go: dev server belum didukung di platform ini\n");
  return 1;
#else
  int port = atoi(spec->settings.port ? spec->settings.port : "8000");
  if (port <= 0 || port > 65535) port = 8000;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "rupa go: dev server socket gagal\n");
    return 1;
  }
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((unsigned short)port);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
      listen(fd, 16) < 0) {
    fprintf(stderr, "rupa go: dev server gagal bind port %d\n", port);
    close(fd);
    return 1;
  }

  printf("rupa go: dev server listening on %s://%s:%d\n",
         spec->settings.protocol ? spec->settings.protocol : "http",
         spec->settings.host ? spec->settings.host : "127.0.0.1", port);
  printf("rupa go: Ctrl+C untuk berhenti\n");
  fflush(stdout);

  static const char resp[] =
      "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 6\r\n"
      "Connection: close\r\n\r\nrupa\n\n";
  for (;;) {
    int client = accept(fd, NULL, NULL);
    if (client < 0) continue;
    /* Baca request minimal (buang) supaya browser tidak retry. */
    char junk[1024];
    ssize_t got = recv(client, junk, sizeof(junk) - 1, 0);
    (void)got;
    size_t sent = 0, len = sizeof(resp) - 1;
    while (sent < len) {
      ssize_t n = send(client, resp + sent, len - sent, 0);
      if (n <= 0) break;
      sent += (size_t)n;
    }
    close(client);
  }
  return 0;
#endif
}

/* ==================== command ==================== */

static void goHelp(void) {
  printf("Usage: rupa go [target] | rupa go <file.rp> [-d]\n\n");
  printf("Menjalankan proyek class ber-konfigurasi .spec; target boleh\n");
  printf("android, web, software, ios (default dari .spec; prioritas web).\n\n");
  printf("Struktur proyek:\n");
  printf("  /app/main.rp    entry — dicari otomatis\n");
  printf("  .spec           konfigurasi proyek\n\n");
  printf("Isi .spec (android):\n");
  printf("  name: your app name\n");
  printf("  author: -\n");
  printf("  version: \"1.0\"\n");
  printf("  target: android\n");
  printf("  arch: unknown\n");
  printf("  debug: false\n");
  printf("  release: false\n\n");
  printf("Isi .spec (web):\n");
  printf("  target: web\n");
  printf("  debug: true\n");
  printf("  release: false\n");
  printf("  domain: example.com\n");
  printf("  settings:\n");
  printf("    - vps:\n");
  printf("      - settingan vps\n");
  printf("    - host: 127.0.0.1\n");
  printf("    - port: 8000\n");
  printf("    - protocol: http\n\n");
  printf("Options:\n");
  printf("  -d    show detail error saat proses compile\n");
}

int goCompile(const char *paths[], int length) {
  /* Konvensi loader (sejajar testDispatch): paths[0..length-1] = argumen
   * setelah "go" — target / file / flag dalam urutan bebas. */
  GoOptions opt = {0};
  const char *explicitPath = NULL;

  for (int i = 0; i < length; i++) {
    const char *a = paths[i];
    if (!strcmp(a, "-h") || !strcmp(a, "--help") || !strcmp(a, "help")) {
      goHelp();
      return 0;
    }
    if (!strcmp(a, "-d")) {
      opt.detail = true;
      continue;
    }
    if (!strcmp(a, "dev") || !strcmp(a, "build")) {
      /* Mode dev/build — target tetap dari .spec (cukup `rupa go dev`). */
      opt.mode = a;
      continue;
    }
    if (a[0] == '[' || strchr(a, ',')) {
      /* Bracket / daftar koma — bisa multi-token. */
      int next = goParseTargets(length, paths, i, &opt);
      if (next < 0) return 1;
      i = next;
      continue;
    }
    /* File eksisten menang atas nama target (edge: file bernama "web"). */
    struct stat st;
    if (!explicitPath && stat(a, &st) == 0 && S_ISREG(st.st_mode)) {
      explicitPath = a;
      continue;
    }
    if (goTargetValid(a)) {
      if (!opt.haveTarget) {
        opt.target = gcstrdup(a);
        opt.haveTarget = true;
      }
      continue;
    }
    /* Bukan file, bukan target: bedakan pesan biar typo terlihat. */
    if (strchr(a, '/') || strstr(a, ".rp"))
      fprintf(stderr, "rupa go: file not found: %s\n", a);
    else
      fprintf(stderr, "rupa go: unknown target '%s' (android, web, software, ios)\n", a);
    return 1;
  }

  GoSpec spec = goSpecLoad();
  /* Kredensial asli ada di .spec terenkripsi — gagal membukanya berarti
   * konfigurasi tidak tersedia; jangan pernah jalan pakai defaults. */
  if (spec.encryptedFailed) return 1;
  if (!opt.haveTarget) {
    opt.target = spec.target;
    opt.haveTarget = true;
  }

  const char *entry = goFindEntry(explicitPath);
  if (!entry) {
    if (explicitPath) {
      fprintf(stderr, "rupa go: file not found: %s\n", explicitPath);
    } else {
      fprintf(stderr,
              "rupa go: main.rp not found (expected app/main.rp or main.rp)\n");
    }
    return 1;
  }
  opt.path = entry;

  /* Mode dev/build (design/rupa_go.txt "Dev or build"): menimpa
   * debug/release .spec. `dev` mencari serve.rp — bila tidak ada,
   * jalankan server default dari .spec.settings. `build` ditunda
   * sampai emisi per-target stabil. */
  bool specDebug = spec.debug;
  bool specRelease = spec.release;
  if (opt.mode && !strcmp(opt.mode, "dev")) {
    specDebug = true;
    specRelease = false;
  } else if (opt.mode && !strcmp(opt.mode, "build")) {
    specDebug = false;
    specRelease = true;
  }

  if (opt.detail || specDebug) {
    printf("rupa go: project=%s version=%s target=%s arch=%s release=%d\n",
           spec.name ? spec.name : "-",
           spec.version ? spec.version : "-",
           opt.target ? opt.target : "-",
           spec.arch ? spec.arch : "-",
           (int)specRelease);
    printf("rupa go: entry=%s\n", opt.path);
    if (opt.mode) printf("rupa go: mode=%s\n", opt.mode);
    if (!strcmp(opt.target ? opt.target : "", "web"))
      printf("rupa go: domain=%s\n", spec.domain ? spec.domain : "-");
    if (spec.haveSettings) {
      printf("rupa go: settings: host=%s port=%s protocol=%s vps=%d\n",
             spec.settings.host ? spec.settings.host : "-",
             spec.settings.port ? spec.settings.port : "-",
             spec.settings.protocol ? spec.settings.protocol : "-",
             spec.settings.vpsCount);
      for (int i = 0; i < spec.settings.vpsCount; i++)
        printf("rupa go:   vps[%d]=%s\n", i, spec.settings.vps[i]);
    }
    /* Database — selalu tampil (kredensial disamarkan; keberadaannya
     * penting untuk verifikasi konfigurasi sebelum emisi). */
    printf("rupa go: db: host=%s port=%s name=%s user=%s pass=%s\n",
           spec.settings.dbhost ? spec.settings.dbhost : "-",
           spec.settings.dbport ? spec.settings.dbport : "-",
           spec.settings.dbname ? spec.settings.dbname : "-",
           spec.settings.dbuser ? spec.settings.dbuser : "-",
           spec.settings.dbpass && strcmp(spec.settings.dbpass, "-") != 0 ? "***"
                                                                          : "-");
  }

  /* Modul `spec from rupa` — expose konfigurasi ke user code
   * (net.connect(spec.settings.host, spec.settings.port)). Object
   * bertag __spec: print ditolak (provenance, lihat value.c). */
  specModuleProvide(specModuleBuild(
      spec.settings.host, spec.settings.port, spec.settings.protocol,
      spec.settings.dbhost, spec.settings.dbport, spec.settings.dbname,
      spec.settings.dbuser, spec.domain));

  int status = 0;
  if (opt.mode && !strcmp(opt.mode, "build")) {
    /* Design: build bisa ditunda — perlu kestabilan bahasa dalam
     * mengolah masing-masing target. Kerangka: pipeline jalan, emisi
     * menyusul. */
    if (opt.detail || specRelease)
      printf("rupa go: build mode — emisi target '%s' menyusul (design rupa_go.txt)\n",
             opt.target ? opt.target : "-");
    status = goRunFile(opt.path, &opt);
  } else if (opt.mode && !strcmp(opt.mode, "dev")) {
    /* dev: serve.rp di project menang — runner penuh dengan hot reload
     * (port milik runner, reload tidak melepas bind). Tanpa serve.rp —
     * main.rp jalan sekali + server default dari .spec.settings. */
    const char *servePath = goFileExists("serve.rp") ? "serve.rp" : NULL;
    if (servePath) {
      int port = atoi(spec.settings.port ? spec.settings.port : "8000");
      if (port <= 0 || port > 65535) port = 8000;
      return serveRun(servePath, port, opt.detail || specDebug);
    }
    status = goRunFile(opt.path, &opt);
    if (status != 0) return status;
    status = goDevServer(&spec);
  } else {
    status = goRunFile(opt.path, &opt);
  }
  if (status != 0) return status;

  /* Compile/emisi per target menyusul — kerangka menjalankan pipeline
   * penuh dulu. */
  return 0;
}
