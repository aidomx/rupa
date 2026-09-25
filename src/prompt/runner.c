#include <rupa.h>

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

  if (!loadCachedFile(index, buffer)) {
    char *path = gcdup(index);
    getcwd(path, MAX_PATH_LENGTH);
    fprintf(stderr, "Message: Cannot read file %s/%s\n", path, index);
    return 1;
  }

  setSourceFilePath(index);

  /* Cache pipeline (caches/pipeline.c): hasil generate + rewrite file
   * yang sama dipakai ulang tanpa lex + parse + rewrite — isi file
   * divalidasi via mtime+nsec+size. Miss = jalur pipeline penuh. */
  Node *node = NULL;
  IRModule *cachedIr = NULL;
  bool pipelineHit = pipelineCacheGet(index, &node, &cachedIr);

  if (!pipelineHit) {
    addToHistory(state);
    addToInput(state);
    lexer(state);

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
    node = processGenerate(&request);

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

      cachedIr = ir;
      pipelineCachePut(index, node, cachedIr);
    }

    int status = executeIRError(cachedIr, node, error);

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
