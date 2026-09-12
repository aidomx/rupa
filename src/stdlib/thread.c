#include <rupa.h>
#include <pthread.h>
#include <time.h>

/* ============================================================
 * Thread Module
 *
 * API:
 *   thread.create(fn) -> handle  — spawn fn in a background thread
 *   thread.join(handle) -> result — wait for thread to finish
 *   thread.sleep(ms)             — sleep current thread
 *   thread.id() -> int           — current thread ID
 *   thread.count() -> int        — number of active threads
 * ============================================================ */

#define MAX_THREADS 64

/* Thread state */
typedef struct {
  pthread_t pthread;
  RuntimeValue result;
  bool done;
  bool has_error;
  char error_msg[256];
} ThreadEntry;

static ThreadEntry threadTable[MAX_THREADS];
static int threadCount = 0;
static pthread_mutex_t tableMutex = PTHREAD_MUTEX_INITIALIZER;

/* Wrapper to pass NativeFn args through pthread_create */
typedef struct {
  RuntimeValue function;
  int argc;
  RuntimeValue *argv;
  ThreadEntry *entry;
} ThreadWrapper;

static void *threadWrapper(void *arg) {
  ThreadWrapper *tw = (ThreadWrapper *)arg;
  InterpreterResult r = resultNormal(valueNull());

  if (tw->function.type == VALUE_NATIVE_FUNCTION &&
      tw->function.as.nativeFunc) {
    r = tw->function.as.nativeFunc->func(tw->argc, tw->argv, NULL, NULL);
  } else if (tw->function.type == VALUE_FUNCTION &&
             tw->function.as.function) {
    RuntimeFunction *fn = tw->function.as.function;
    RuntimeEnv *local = semCreateEnv(fn->closure);
    if (!local) {
      r = resultFlow(FLOW_ERROR, valueString("Failed to create thread environment"));
    } else {
      for (int i = 0; i < tw->argc && i < fn->paramLength; i++) {
        const char *name = NULL;
        int param = fn->params[i];
        if (param >= 0 && param < fn->node->length) {
          AstNode *pn = &fn->node->ast[param];
          if (pn->type == NODE_IDENTIFIER) name = pn->identifier.name;
          else if (pn->type == NODE_LITERAL_ID) name = pn->string.value;
        }
        if (name) semSet(local, name, tw->argv[i]);
      }
      r = interpretNode(fn->node, fn->body, local, NULL);
      if (r.flow == FLOW_RETURN) r = resultNormal(r.value);
    }
  } else {
    r = resultFlow(FLOW_ERROR, valueString("Invalid thread function"));
  }

  pthread_mutex_lock(&tableMutex);
  tw->entry->result = r.value;
  tw->entry->done = true;
  if (r.flow == FLOW_ERROR) {
    tw->entry->has_error = true;
    snprintf(tw->entry->error_msg, sizeof(tw->entry->error_msg),
             "Thread error");
  }
  pthread_mutex_unlock(&tableMutex);

  free(tw->argv);
  free(tw);
  return NULL;
}

/* ==================== thread.create(fn) ==================== */
static InterpreterResult threadCreate(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  if (argc < 1 ||
      (argv[0].type != VALUE_NATIVE_FUNCTION && argv[0].type != VALUE_FUNCTION))
    return resultFlow(FLOW_ERROR,
                      valueString("thread.create() expects a function"));

  pthread_mutex_lock(&tableMutex);
  if (threadCount >= MAX_THREADS) {
    pthread_mutex_unlock(&tableMutex);
    return resultFlow(FLOW_ERROR, valueString("Too many threads"));
  }

  ThreadEntry *entry = &threadTable[threadCount];
  int id = threadCount;
  threadCount++;
  pthread_mutex_unlock(&tableMutex);

  entry->done = false;
  entry->has_error = false;
  entry->result = valueNull();
  entry->error_msg[0] = '\0';

  ThreadWrapper *tw = malloc(sizeof(ThreadWrapper));
  if (!tw) return resultFlow(FLOW_ERROR, valueString("malloc failed"));
  tw->function = argv[0];
  tw->argc = 0;
  tw->argv = NULL;
  tw->entry = entry;

  int rc = pthread_create(&entry->pthread, NULL, threadWrapper, tw);
  if (rc != 0) {
    free(tw);
    return resultFlow(FLOW_ERROR, valueString("Failed to create thread"));
  }

  return resultNormal(valueNumber(id));
}

/* ==================== thread.join(handle) ==================== */
static InterpreterResult threadJoin(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_NUMBER)
    return resultFlow(FLOW_ERROR,
                      valueString("thread.join() expects a thread handle"));

  int id = argv[0].as.number;
  pthread_mutex_lock(&tableMutex);
  if (id < 0 || id >= threadCount) {
    pthread_mutex_unlock(&tableMutex);
    return resultFlow(FLOW_ERROR, valueString("Invalid thread handle"));
  }
  ThreadEntry *entry = &threadTable[id];
  pthread_mutex_unlock(&tableMutex);

  pthread_join(entry->pthread, NULL);

  if (entry->has_error)
    return resultFlow(FLOW_ERROR, valueString(entry->error_msg));

  return resultNormal(entry->result);
}

/* ==================== thread.sleep(ms) ==================== */
static InterpreterResult threadSleep(int argc, RuntimeValue *argv,
                                     RuntimeEnv *env, Error *error) {
  if (argc < 1)
    return resultFlow(FLOW_ERROR,
                      valueString("thread.sleep() expects milliseconds"));

  long ms = 0;
  if (argv[0].type == VALUE_NUMBER)
    ms = argv[0].as.number;
  else if (argv[0].type == VALUE_DECIMAL)
    ms = (long)argv[0].as.decimal;
  else
    return resultFlow(FLOW_ERROR,
                      valueString("thread.sleep() expects a number"));

  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);

  return resultNormal(valueNull());
}

/* ==================== thread.id() ==================== */
static InterpreterResult threadId(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  /* Return a hash of the current pthread_t as an integer */
  pthread_t self = pthread_self();
  long id = 0;
  unsigned char *p = (unsigned char *)&self;
  for (size_t i = 0; i < sizeof(pthread_t); i++)
    id = id * 31 + p[i];
  return resultNormal(valueNumber((int)(id & 0x7FFFFFFF)));
}

/* ==================== thread.count() ==================== */
static InterpreterResult threadCountFn(int argc, RuntimeValue *argv,
                                       RuntimeEnv *env, Error *error) {
  pthread_mutex_lock(&tableMutex);
  int active = 0;
  for (int i = 0; i < threadCount; i++)
    if (!threadTable[i].done) active++;
  pthread_mutex_unlock(&tableMutex);
  return resultNormal(valueNumber(active));
}

/* ==================== Module init ==================== */
static void addEntry(struct RuntimeObjectEntry **head, const char *name,
                     NativeFn fn, int paramCount) {
  struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
  e->key = strdup(name);
  e->value = valueNativeFunction(name, fn, paramCount);
  e->next = *head;
  *head = e;
}

InterpreterResult stdThreadInit(Node *node, int id, RuntimeEnv *env,
                                Error *error) {
  struct RuntimeObjectEntry *entries = NULL;

  addEntry(&entries, "create", threadCreate, 1);
  addEntry(&entries, "join", threadJoin, 1);
  addEntry(&entries, "sleep", threadSleep, 1);
  addEntry(&entries, "id", threadId, 0);
  addEntry(&entries, "count", threadCountFn, 0);

  return resultNormal(valueObject(entries));
}
