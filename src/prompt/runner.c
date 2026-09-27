#include <rupa.h>

#if defined(RUPA_WINDOWS)
#include <io.h>
#endif

typedef struct {
  char *path;
  char *content;
  size_t length;
  time_t mtime;
  long mtime_nsec;
  off_t size;
} FileCache;

static FileCache *fileCaches = NULL;
static int fileCacheCount = 0;

typedef struct {
  double total;
  double cache;
  double load;
  double lex;
  double parse;
  double rewrite;
  double execute;
  bool cacheHit;
} ProfileStats;

static ProfileStats *activeProfile = NULL;

static double profileNow(void) {
#if defined(_WIN32)
  return (double)clock() / (double)CLOCKS_PER_SEC;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static void profileAdd(double *slot, double start) {
  if (activeProfile && slot) *slot += profileNow() - start;
}

/* --- Suppress stdout untuk mode profile (silent, hanya laporan) ---
 * Program yang diukur tetap dieksekusi penuh (print, dll), tetapi
 * outputnya dialihkan ke /dev/null (NUL di Windows) supaya laporan
 * profiler bersih. stderr dibiarkan: error runtime tetap terlihat. */
static int g_savedStdout = -1;

static void runnerStdoutSuppress(void) {
  fflush(stdout);
#if defined(RUPA_WINDOWS)
  g_savedStdout = _dup(_fileno(stdout));
  int devnull = _open("NUL", _O_WRONLY);
#else
  g_savedStdout = dup(fileno(stdout));
  int devnull = open("/dev/null", O_WRONLY);
#endif
  if (devnull >= 0) {
    if (g_savedStdout >= 0)
#if defined(RUPA_WINDOWS)
      _dup2(devnull, _fileno(stdout));
#else
      dup2(devnull, fileno(stdout));
#endif
#if defined(RUPA_WINDOWS)
    _close(devnull);
#else
    close(devnull);
#endif
  }
}

static void runnerStdoutRestore(void) {
  if (g_savedStdout < 0) return;
  fflush(stdout);
#if defined(RUPA_WINDOWS)
  _dup2(g_savedStdout, _fileno(stdout));
  _close(g_savedStdout);
#else
  dup2(g_savedStdout, fileno(stdout));
  close(g_savedStdout);
#endif
  g_savedStdout = -1;
}

/* stderr: disuppress HANYA saat warmup — error (lexer/parser/runtime)
 * sudah tercetak measured run; tanpa ini laporan dobel. */
static int g_savedStderr = -1;

static void runnerStderrSuppress(void) {
  fflush(stderr);
#if defined(RUPA_WINDOWS)
  g_savedStderr = _dup(_fileno(stderr));
  int devnull = _open("NUL", _O_WRONLY);
#else
  g_savedStderr = dup(fileno(stderr));
  int devnull = open("/dev/null", O_WRONLY);
#endif
  if (devnull >= 0) {
    if (g_savedStderr >= 0)
#if defined(RUPA_WINDOWS)
      _dup2(devnull, _fileno(stderr));
#else
      dup2(devnull, fileno(stderr));
#endif
#if defined(RUPA_WINDOWS)
    _close(devnull);
#else
    close(devnull);
#endif
  }
}

static void runnerStderrRestore(void) {
  if (g_savedStderr < 0) return;
  fflush(stderr);
#if defined(RUPA_WINDOWS)
  _dup2(g_savedStderr, _fileno(stderr));
  _close(g_savedStderr);
#else
  dup2(g_savedStderr, fileno(stderr));
  close(g_savedStderr);
#endif
  g_savedStderr = -1;
}

static void profilePrint(const char *path, int status, const ProfileStats *p) {
  double total = p ? p->total : 0.0;
  printf("Rupa Profiler v1.0\n\n");
  printf("File       : %s\n", path ? path : "-");
  printf("Status     : %s\n", status == 0 ? "ok" : "failed");
  printf("Cache      : %s\n\n", p && p->cacheHit ? "hit" : "miss");
  printf("STAGE          TIME        %%\n");
  printf("--------------------------------\n");
  if (p) {
    printf("cache        %8.3f ms  %5.1f%%\n", p->cache * 1000.0, total > 0 ? p->cache / total * 100.0 : 0.0);
    printf("load         %8.3f ms  %5.1f%%\n", p->load * 1000.0, total > 0 ? p->load / total * 100.0 : 0.0);
    printf("lexer        %8.3f ms  %5.1f%%\n", p->lex * 1000.0, total > 0 ? p->lex / total * 100.0 : 0.0);
    printf("parser       %8.3f ms  %5.1f%%\n", p->parse * 1000.0, total > 0 ? p->parse / total * 100.0 : 0.0);
    printf("rewrite      %8.3f ms  %5.1f%%\n", p->rewrite * 1000.0, total > 0 ? p->rewrite / total * 100.0 : 0.0);
    printf("execute      %8.3f ms  %5.1f%%\n", p->execute * 1000.0, total > 0 ? p->execute / total * 100.0 : 0.0);
    printf("--------------------------------\n");
    printf("total        %8.3f ms  100.0%%\n", total * 1000.0);
  }
}

static int isDirectory(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  return S_ISDIR(st.st_mode);
}

static long getMtimeNsec(const struct stat *st) {
#if defined(__APPLE__)
  return st->st_mtimespec.tv_nsec;
#elif defined(_WIN32)
  return 0;
#else
  return st->st_mtim.tv_nsec;
#endif
}

static FileCache *findFileCache(const char *path) {
  for (int i = 0; i < fileCacheCount; i++) {
    if (strcmp(fileCaches[i].path, path) == 0)
      return &fileCaches[i];
  }

  return NULL;
}

static int loadCachedFile(const char *path, Buffer *buffer) {
  struct stat st;

  if (stat(path, &st) != 0)
    return 0;

  FileCache *cache = findFileCache(path);

  /*
   * File belum berubah:
   * tidak perlu menyentuh disk untuk membaca ulang isinya.
   */
  if (cache &&
      cache->mtime == st.st_mtime &&
      cache->mtime_nsec == getMtimeNsec(&st) &&
      cache->size == st.st_size) {

    if (cache->length >= (size_t)buffer->capacity)
      return 0;

    memcpy(buffer->value, cache->content, cache->length);
    buffer->value[cache->length] = '\0';
    buffer->length = (int)cache->length;

    return 1;
  }

  /*
   * File baru atau berubah.
   */
  if (!readfile(path, buffer))
    return 0;

  if (!cache) {
    FileCache *next = realloc(
      fileCaches,
      sizeof(FileCache) * (fileCacheCount + 1)
    );

    if (!next)
      return 0;

    fileCaches = next;
    cache = &fileCaches[fileCacheCount++];

    memset(cache, 0, sizeof(*cache));

    cache->path = strdup(path);
    if (!cache->path)
      return 0;
  }

  char *content = realloc(cache->content, (size_t)buffer->length + 1);
  if (!content)
    return 0;

  cache->content = content;
  cache->length = (size_t)buffer->length;

  memcpy(cache->content, buffer->value, cache->length);
  cache->content[cache->length] = '\0';

  cache->mtime = st.st_mtime;
  cache->mtime_nsec = getMtimeNsec(&st);
  cache->size = st.st_size;

  return 1;
}

int run(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No such file for execute.\n");
    return 1;
  }

  /* File mode: bukan REPL. */
  State *state = createGlobalState(length, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return 1;
  }

  const char *index = paths[length];

  bool runWithIR = true;

  if (isDirectory(index)) {
    static char indexBuf[1024];
    snprintf(indexBuf, sizeof(indexBuf), "%s/index.rp", index);
    index = indexBuf;
  }

  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;
  analyzerReset();

  Buffer *buffer = state->buffer;

  double profileStart = activeProfile ? profileNow() : 0.0;
  if (!loadCachedFile(index, buffer)) {
    char *path = gcdup(index);
    getcwd(path, MAX_PATH_LENGTH);
    fprintf(stderr, "Message: Cannot read file %s/%s\n", path, index);
    return 1;
  }
  if (activeProfile) profileAdd(&activeProfile->load, profileStart);

  setSourceFilePath(index);

  /* Cache pipeline (caches/pipeline.c): hasil generate + rewrite file
   * yang sama dipakai ulang tanpa lex + parse + rewrite — isi file
   * divalidasi via mtime+nsec+size. Miss = jalur pipeline penuh. */
  Node *node = NULL;
  IRModule *cachedIr = NULL;
  profileStart = activeProfile ? profileNow() : 0.0;
  bool pipelineHit = pipelineCacheGet(index, &node, &cachedIr);
  if (activeProfile) profileAdd(&activeProfile->cache, profileStart);
  if (activeProfile) activeProfile->cacheHit = pipelineHit;

  if (!pipelineHit) {
    addToHistory(state);
    addToInput(state);
    profileStart = activeProfile ? profileNow() : 0.0;
    lexer(state);
    if (activeProfile) profileAdd(&activeProfile->lex, profileStart);

    Flags *flags = state->input->flags;
    Token *tokens = state->tokens;

    if (!tokens || tokens->length == 0 || (flags && flags->isWaiting)) {
      if (!state->error || state->error->size == 0)
        addSourceErrorAt(
          state->error,
          "LexerError",
          "incomplete or invalid input",
          state->input->content,
          state->input->cursor,
          ERR_UNEXPECTED_EOF
        );

      printErrors(state->error);
      return 1;
    }

    Request request = createRequestWithError(tokens, 10, state->error);
    profileStart = activeProfile ? profileNow() : 0.0;
    node = processGenerate(&request);
    if (activeProfile) profileAdd(&activeProfile->parse, profileStart);

    /* SyntaxError dari parser (unexpected token, dll) = fatal meski
     * statement lain sukses — tanpa ini error dibuang diam-diam dan
     * program jalan parsial (hasAstDeclarations hanya gate all-fail). */
    if (state->error && state->error->size > 0) {
      printErrors(state->error);
      return 1;
    }

    if (!node || node->length <= 0 || !hasAstDeclarations(tokens)) {
      if (!state->error || state->error->size == 0)
        addSourceErrorAt(
          state->error,
          "ParserError",
          "no AST declarations produced",
          state->input->content,
          state->input->cursor,
          ERR_SYNTAX
        );

      printErrors(state->error);
      return 1;
    }

    pipelineCachePut(index, node, NULL);
  }

  Error *error = createError(10);

  int root = -1;

  for (int j = 0; j < node->length; j++) {
    if (node->ast[j].type == NODE_PROGRAM) {
      root = j;
      break;
    }
  }

  if (root < 0) {
    addSourceErrorAt(
      state->error,
      "ParserError",
      "program root not found",
      state->input->content,
      state->input->cursor,
      ERR_SYNTAX
    );

    printErrors(state->error);
    return 1;
  }

  if (runWithIR) {
    /* IR: dari cache bila entry sudah lengkap; bila tidak, bangun
     * dari AST (yang mungkin dari cache) lalu upgrade entry. */
    if (!cachedIr) {
      IRModule *ir = createIR();

      profileStart = activeProfile ? profileNow() : 0.0;
      if (!ir || !rewrite(node, -1, ir)) {
        addSourceErrorAt(
          state->error,
          "IRError",
          "rewrite ast to ir is failed.",
          state->input->content,
          state->input->cursor,
          ERR_SYNTAX
        );

        printErrors(state->error);
        return 1;
      }

      if (activeProfile) profileAdd(&activeProfile->rewrite, profileStart);
      cachedIr = ir;
      pipelineCachePut(index, node, cachedIr);
    }

    profileStart = activeProfile ? profileNow() : 0.0;
    int status = executeIRError(cachedIr, node, error);
    if (activeProfile) profileAdd(&activeProfile->execute, profileStart);

    /* cachedIr dimiliki cache pipeline — tidak di-free di sini. */

    if (status != 0)
      printErrors(error);

    return status;
  }

  RuntimeEnv *env = semCreateEnv(NULL);
  if (!env)
    return 1;

  stdlibInit(env);
  builtinsInit(env);

  extern struct EventLoop *g_event_loop;
  g_event_loop = eventLoopCreate();

  InterpreterResult result =
    interpretNode(node, root, env, error);

  for (int i = 0;
       i < 1000 && eventLoopHasPending(g_event_loop);
       i++) {
    eventLoopRun(node, g_event_loop, env, error);
  }

  if (error && error->size > 0)
    printErrors(error);

  eventLoopDestroy(g_event_loop);
  g_event_loop = NULL;

  return result.flow == FLOW_ERROR ||
         (error && error->size > 0) ? 1 : 0;
}

int profileRun(const char *paths[], int length) {
  if (!paths || length != 1 || !paths[0]) {
    fprintf(stderr, "rupa profile: usage: rupa profile <file.rp>\n");
    return 1;
  }

  /* run() mengambil entry file dari paths[length]. Bentuk command biasa
   * membawa argv[argc] sebagai sentinel, tetapi profile menerima slice argv
   * dari loader. Buat sentinel lokal yang menunjuk file target. */
  const char *runPaths[2] = {paths[0], paths[0]};

  ProfileStats stats = {0};

  /* Warmup senyap TANPA profiler (activeProfile = NULL): isi pipeline
   * cache (AST+IR) + FileCache isi file. Cache pipeline ini in-process —
   * tanpa warmup, Cache: selalu miss karena satu file dieksekusi tepat
   * satu kali per proses (tidak ada kesempatan hit). Warmup mengeksekusi
   * program sekali dengan stdout dialihkan ke /dev/null (profile =
   * silent, hanya laporan). */
  runnerStdoutSuppress();
  runnerStderrSuppress();
  (void)run(runPaths, 1);
  runnerStderrRestore();
  runnerStdoutRestore();

  /* Measured run: Cache: hit — lexer+parser+rewrite tercabut dari
   * pengukuran (pipeline cache), yang tersisa cache lookup + load
   * + execute. Program tetap silent (stdout tertahan). Profiler aktif
   * HANYA di run ini supaya stage warmup tidak mencemari stats. */
  activeProfile = &stats;
  double totalStart = profileNow();
  runnerStdoutSuppress();
  int status = run(runPaths, 1);
  runnerStdoutRestore();
  stats.total = profileNow() - totalStart;
  activeProfile = NULL;

  profilePrint(paths[0], status, &stats);
  return status;
}

void execute(const char *code) {
  if (!code || strlen(code) == 0) {
    fprintf(stderr, "No code provided.\n");
    return;
  }

  State *state = createGlobalState(10, true);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create state.\n");
    return;
  }

  state->isRepl = false;
  clearReplState(state->repl);
  clearInput(state->input);
  clearStateToken(state->tokens);
  clearStateContext(state->context);
  state->size = 0;
  analyzerReset();

  Buffer *buffer = state->buffer;
  size_t len = strlen(code);

  if ((int)len >= buffer->capacity) {
    fprintf(stderr, "Code is too long.\n");
    return;
  }

  memcpy(buffer->value, code, len);
  buffer->value[len] = '\0';
  buffer->length = (int)len;

  processInput(state);
}
