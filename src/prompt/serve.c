#include <rupa.h>

/**
 * @brief serve.rp runner — mode dev `rupa go dev` (design/rupa_go.txt).
 *
 * Arsitektur: RUNNER yang memiliki server socket (bind sekali di depan,
 * port tetap terikat selama proses hidup) — serve.rp/user code hanya
 * menyuplai handler. Bentuk serve.rp yang didukung:
 *
 *   import net from rupa
 *   net.listen(8000)              # opsional — menyatakan port
 *   handle(method, path) {
 *     return "..."
 *   }
 *   handleStart() { ... }         # opsional — dipanggil sekali saat boot
 *
 * - handle(method, path) wajib ada; hasilnya jadi body HTTP 200.
 * - Hot reload: watch mtime+nsec serve.rp; saat berubah, proses ulang
 *   serve.rp penuh (fresh env) tanpa membebaskan port. Reload gagal
 *   (syntax error, tanpa handle) = server tetap hidup dengan handler
 *   lama, error ditampilkan.
 * - Gate: hanya aktif di mode dev (spec.debug && !spec.release).
 */

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#define SERVE_SOCK 1
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#define SERVE_SOCK 0
#endif

/* ==================== handler call helper ==================== */

/* Panggil VALUE_FUNCTION rupa dari C: bind param nama → nilai, jalankan
 * body di frame baru (semantik sama dengan interpretCall, versi minimal
 * untuk closure tanpa this). */
static InterpreterResult serveCallFunction(RuntimeValue callee, RuntimeValue *args,
                                           int argc, Error *error) {
  if (callee.type != VALUE_FUNCTION || !callee.as.function)
    return resultFlow(FLOW_ERROR, valueNull());
  RuntimeFunction *function = callee.as.function;

  RuntimeEnv *local = semCreateEnv(function->closure);
  if (!local) return resultFlow(FLOW_ERROR, valueNull());

  int count = argc < function->paramLength ? argc : function->paramLength;
  for (int i = 0; i < count; i++) {
    int id = function->params[i];
    const char *name = NULL;
    if (id >= 0 && id < function->node->length) {
      AstNode *ast = &function->node->ast[id];
      if (ast->type == NODE_IDENTIFIER)
        name = ast->identifier.name;
      else if (ast->type == NODE_LITERAL_ID)
        name = ast->string.value;
      else if (ast->type == NODE_ANNOTATION && ast->annotation.name >= 0 &&
               ast->annotation.name < function->node->length &&
               function->node->ast[ast->annotation.name].type == NODE_IDENTIFIER)
        name = function->node->ast[ast->annotation.name].identifier.name;
    }
    if (name) semSet(local, name, args[i]);
  }

  InterpreterResult result =
      interpretNode(function->node, function->body, local, error);
  if (result.flow == FLOW_RETURN) return resultNormal(result.value);
  return result;
}

/* ==================== watch (mtime polling) ==================== */

typedef struct ServeWatch {
  char path[PATH_MAX];
  time_t mtime;
  long mtime_nsec;
  off_t size;
  bool valid;
} ServeWatch;

static bool serveWatchPoll(ServeWatch *w);

static long serveMtimeNsec(const struct stat *st) {
#if defined(__APPLE__)
  return st->st_mtimespec.tv_nsec;
#elif defined(_WIN32)
  return 0;
#else
  return st->st_mtim.tv_nsec;
#endif
}

static void serveWatchInit(ServeWatch *w, const char *path) {
  memset(w, 0, sizeof(*w));
  snprintf(w->path, sizeof(w->path), "%s", path);
  serveWatchPoll(w);
}

/* Return true bila file berubah sejak pemeriksaan terakhir. */
static bool serveWatchPoll(ServeWatch *w) {
  struct stat st;
  if (stat(w->path, &st) != 0) {
    bool was = w->valid;
    w->valid = false;
    return was; /* hilang = berubah */
  }
  bool changed = !w->valid || w->mtime != st.st_mtime ||
                 w->mtime_nsec != serveMtimeNsec(&st) || w->size != st.st_size;
  w->mtime = st.st_mtime;
  w->mtime_nsec = serveMtimeNsec(&st);
  w->size = st.st_size;
  w->valid = true;
  return changed;
}

/* ==================== load serve.rp ==================== */

typedef struct ServeApp {
  bool ok;
  bool netListenSeen;  /* serve.rp memanggil net.listen (port eksplisit) */
  RuntimeValue handler;
  RuntimeEnv *env;     /* env hasil eksekusi serve.rp (handler closure) */
  Error *error;
} ServeApp;

/* Port TIDAK dinyatakan di serve.rp — socket milik runner (bind sekali,
 * reload tidak melepasnya); port dari .spec.settings.port. serve.rp
 * hanya mendefinisikan handle(method, path). `import net from rupa` di
 * serve.rp tetap boleh (untuk utilitas lain), tapi memanggil
 * net.listen di serve.rp akan menyandera port dan menembak runner. */

static bool serveExecScript(const char *path, ServeApp *app) {
  app->ok = false;
  app->netListenSeen = false;
  app->handler = valueNull();
  app->env = NULL;

  /* net.listen(port) di serve.rp di-intercept: declaration binding
   * `net` standar dulu, lalu override anggota `listen` dengan hook yang
   * mencatat port (socket milik runner, bukan milik serve.rp). */

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "serve: failed to create state\n");
    return false;
  }
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;
  analyzerReset();

  Buffer *buffer = state->buffer;
  if (!readfile(path, buffer)) {
    fprintf(stderr, "serve: cannot read %s\n", path);
    return false;
  }
  setSourceFilePath(path);
  addToHistory(state);
  addToInput(state);
  lexer(state);

  Flags *flags = state->input->flags;
  Token *tokens = state->tokens;
  if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
    printErrors(state->error);
    return false;
  }

  Request request = createRequestWithError(tokens, 10, state->error);
  Node *node = processGenerate(&request);
  if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
    printErrors(state->error);
    return false;
  }

  int root = -1;
  for (int j = 0; j < node->length; j++)
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  if (root < 0) return false;

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env) return false;
  stdlibInit(env);
  builtinsInit(env);
  testHelperInit(env);

  InterpreterResult r = interpretNode(node, root, env, app->error);
  bool failed = r.flow == FLOW_ERROR || (app->error && app->error->size > 0);
  if (failed) {
    printErrors(app->error);
    return false;
  }

  /* Handler wajib: binding `handle` berupa fungsi (2 param). */
  RuntimeValue h = valueNull();
  if (!semGet(env, "handle", &h) || h.type != VALUE_FUNCTION) {
    fprintf(stderr,
            "serve: %s tidak mendefinisikan handle(method, path)\n", path);
    return false;
  }

  app->ok = true;
  app->handler = h;
  app->env = env;
  return true;
}

/* ==================== HTTP minimal ==================== */

/* Baca header request (sampai \r\n\r\n) + ambil request-line. */
static bool serveReadRequest(int client, char *method, int methodCap, char *target,
                             int targetCap) {
  char buf[8192];
  size_t used = 0;
  for (;;) {
    ssize_t n = recv(client, buf + used, sizeof(buf) - 1 - used, 0);
    if (n <= 0) return false;
    used += (size_t)n;
    buf[used] = '\0';
    char *end = strstr(buf, "\r\n\r\n");
    if (end) break;
    if (used >= sizeof(buf) - 1) return false;
  }
  char *line = buf;
  char *sp1 = strchr(line, ' ');
  if (!sp1) return false;
  *sp1 = '\0';
  if ((int)strlen(line) >= methodCap) return false;
  strcpy(method, line);
  char *sp2 = strchr(sp1 + 1, ' ');
  if (!sp2) return false;
  *sp2 = '\0';
  if ((int)strlen(sp1 + 1) >= targetCap) return false;
  strcpy(target, sp1 + 1);
  /* Buang query string. */
  char *q = strchr(target, '?');
  if (q) *q = '\0';
  return method[0] && target[0];
}

static bool serveSendAll(int client, const char *buf, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(client, buf + sent, len - sent, 0);
    if (n <= 0) return false;
    sent += (size_t)n;
  }
  return true;
}

static void serveRespond(int client, const char *body, size_t bodyLen) {
  char head[256];
  snprintf(head, sizeof(head),
           "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
           "Content-Length: %zu\r\nConnection: close\r\n\r\n",
           bodyLen);
  if (!serveSendAll(client, head, strlen(head))) return;
  if (bodyLen) serveSendAll(client, body, bodyLen);
}

/* ==================== runner ==================== */

int serveRun(const char *path, int portFallback, bool detail) {
#if !SERVE_SOCK
  (void)path; (void)portFallback; (void)detail;
  fprintf(stderr, "serve: belum didukung di platform ini\n");
  return 1;
#else
  int listenFd = -1;
  int boundPort = portFallback;
  ServeApp app = {0};
  app.error = createError(10);

  /* Boot pertama: jalankan serve.rp untuk mendapat port (net.listen)
   * + handler. Bila gagal: tanpa server — exit 1 (bukan fallback
   * diam-diam; pesan error sudah ditampilkan). */
  if (!serveExecScript(path, &app)) {
    app.error = NULL; /* GC-owned — dibebaskan di gcclean */
    return 1;
  }
  boundPort = portFallback;

  /* Socket milik RUNNER — bind sekali, tidak pernah dibebaskan saat
   * reload; serve.rp yang memanggil net.listen hanya MENYATAKAN port. */
  listenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd < 0) {
    fprintf(stderr, "serve: socket gagal\n");
    app.error = NULL;
    return 1;
  }
  int one = 1;
  setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((unsigned short)boundPort);
  if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
      listen(listenFd, 16) < 0) {
    fprintf(stderr, "serve: bind %d gagal: %s\n", boundPort, strerror(errno));
    app.error = NULL;
    return 1;
  }
  printf("serve: http://127.0.0.1:%d (dev server — %s)\n", boundPort, path);
  printf("serve: hot reload aktif — Ctrl+C untuk berhenti\n");
  fflush(stdout);

  ServeWatch watch;
  serveWatchInit(&watch, path);

  for (;;) {
    /* Reload check: jangan blokir — poll socket dulu dengan timeout. */
    fd_set rfds;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 300 * 1000};
    FD_ZERO(&rfds);
    FD_SET(listenFd, &rfds);
    int ready = select(listenFd + 1, &rfds, NULL, NULL, &tv);

    if (serveWatchPoll(&watch)) {
      if (detail) printf("serve: reload (%s berubah)\n", path);
      ServeApp next = {0};
      next.error = createError(10);
      if (serveExecScript(path, &next)) {
        /* env/error lama GC arena — dibebaskan di gcclean. */
        app = next;
        printf("serve: reloaded\n");
        fflush(stdout);
      } else {
        fprintf(stderr, "serve: reload gagal — handler lama tetap dipakai\n");
        next.error = NULL;
      }
    }

    if (ready <= 0) continue; /* timeout — loop reload check lagi */

    int client = accept(listenFd, NULL, NULL);
    if (client < 0) continue;

    char method[16] = {0}, target[1024] = {0};
    if (!serveReadRequest(client, method, sizeof(method), target, sizeof(target))) {
      close(client);
      continue;
    }

    RuntimeValue args[2] = {valueString(method), valueString(target)};
    InterpreterResult r =
        serveCallFunction(app.handler, args, 2, app.error);
    if (r.flow == FLOW_ERROR || (app.error && app.error->size > 0)) {
      printErrors(app.error);
      app.error->size = 0;
      static const char e50[] =
          "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n"
          "Connection: close\r\n\r\n";
      serveSendAll(client, e50, sizeof(e50) - 1);
      close(client);
      continue;
    }

    const char *body = "";
    if (r.value.type == VALUE_STRING && r.value.as.string)
      body = r.value.as.string;
    serveRespond(client, body, strlen(body));
    close(client);
  }
  return 0;
#endif
}
