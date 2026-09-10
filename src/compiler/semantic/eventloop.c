#include <rupa.h>
#include <time.h>

extern struct EventLoop *g_event_loop;

static long currentTimeMs(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static RuntimeValue makeHandleObj(const char *status, RuntimeValue data, RuntimeValue errVal) {
  struct RuntimeObjectEntry *head = NULL;
  struct RuntimeObjectEntry *tail = NULL;

  const char *keys[] = {"status", "data", "error"};
  RuntimeValue vals[] = {valueString(status), data, errVal};

  for (int i = 0; i < 3; i++) {
    struct RuntimeObjectEntry *e = gcmall(sizeof(*e));
    if (!e) continue;
    e->key = gcstrdup(keys[i]);
    e->value = vals[i];
    e->next = NULL;
    if (tail) {
      tail->next = e;
      tail = e;
    } else {
      head = tail = e;
    }
  }

  return valueObject(head);
}

struct EventLoop *eventLoopCreate(void) {
  struct EventLoop *loop = gcmall(sizeof(*loop));
  if (loop) {
    loop->head = NULL;
    loop->tail = NULL;
    loop->count = 0;
  }
  return loop;
}

void eventLoopPush(struct EventLoop *loop, int handleId, int requestId, int handlerId,
                   int timeoutMs, int loaderId, int timeoutId) {
  if (!loop) return;
  struct AsyncEvent *e = gcmall(sizeof(*e));
  if (!e) return;
  e->handleId = handleId;
  e->requestId = requestId;
  e->handlerId = handlerId;
  e->timeoutMs = timeoutMs;
  e->loaderId = loaderId;
  e->timeoutId = timeoutId;
  e->startTime = currentTimeMs();
  e->result = valueNull();
  e->state = ASYNC_PENDING;
  e->pollCount = 0;
  e->next = NULL;

  if (loop->tail) {
    loop->tail->next = e;
    loop->tail = e;
  } else {
    loop->head = loop->tail = e;
  }
  loop->count++;
}

static void callHandler(Node *node, int handlerId, RuntimeEnv *env, Error *error,
                        RuntimeValue thisVal) {
  if (handlerId < 0 || handlerId >= node->length) return;
  semSet(env, "this", thisVal);
  interpretNode(node, handlerId, env, error);
}

static void callLoader(Node *node, int loaderId, RuntimeEnv *env, Error *error,
                       RuntimeValue thisVal) {
  if (loaderId < 0) return;
  semSet(env, "this", thisVal);
  int callId = createCall(node, loaderId, NULL, 0);
  if (callId >= 0) {
    interpretNode(node, callId, env, error);
  }
}

/**
 * @brief Process one async event — evaluate request, call handler.
 *
 * Used for background async operations that need event loop polling.
 */
static void processEvent(struct AsyncEvent *e, Node *node, RuntimeEnv *env, Error *error) {
  if (e->state == ASYNC_DONE) return;

  /* Check timeout */
  if (e->timeoutMs > 0) {
    long elapsed = currentTimeMs() - e->startTime;
    if (elapsed >= e->timeoutMs) {
      e->state = ASYNC_ERROR;
      e->result = valueNull();
      if (e->loaderId >= 0)
        callLoader(node, e->loaderId, env, error,
                   makeHandleObj("ERROR", valueNull(), valueString("Timed out")));
      else if (e->handlerId >= 0)
        callHandler(node, e->handlerId, env, error,
                    makeHandleObj("ERROR", valueNull(), valueString("Timed out")));
      e->state = ASYNC_DONE;
      return;
    }
  }

  /* Evaluate request */
  InterpreterResult r = interpretNode(node, e->requestId, env, error);
  if (r.flow == FLOW_ERROR) {
    e->state = ASYNC_ERROR;
    e->result = valueNull();
    if (e->loaderId >= 0)
      callLoader(node, e->loaderId, env, error, makeHandleObj("ERROR", valueNull(), r.value));
    else if (e->handlerId >= 0)
      callHandler(node, e->handlerId, env, error, makeHandleObj("ERROR", valueNull(), r.value));
  } else {
    e->result = r.value;
    e->state = ASYNC_SUCCESS;
    if (e->loaderId >= 0)
      callLoader(node, e->loaderId, env, error, makeHandleObj("SUCCESS", e->result, valueNull()));
    else if (e->handlerId >= 0)
      callHandler(node, e->handlerId, env, error, makeHandleObj("SUCCESS", e->result, valueNull()));
  }
  e->state = ASYNC_DONE;
}

/**
 * @brief Run event loop — process all pending events once.
 */
void eventLoopRun(Node *node, struct EventLoop *loop, RuntimeEnv *env, Error *error) {
  if (!loop) return;

  for (struct AsyncEvent *e = loop->head; e; e = e->next) {
    if (e->state == ASYNC_DONE) continue;
    processEvent(e, node, env, error);
  }
}

/**
 * @brief Run event loop until a specific handle is done.
 */
void eventLoopRunUntil(struct EventLoop *loop, int handleId, Node *node, RuntimeEnv *env,
                       Error *error) {
  if (!loop) return;

  for (int i = 0; i < 1000; i++) {
    bool found = false;
    for (struct AsyncEvent *e = loop->head; e; e = e->next) {
      if (e->handleId == handleId) {
        if (e->state == ASYNC_DONE) return;
        found = true;
        break;
      }
    }
    if (!found) return;

    eventLoopRun(node, loop, env, error);
  }
}

bool eventLoopGetResult(struct EventLoop *loop, int handleId, RuntimeValue *out) {
  if (!loop) return false;
  for (struct AsyncEvent *e = loop->head; e; e = e->next)
    if (e->handleId == handleId && e->state == ASYNC_DONE) {
      if (out) *out = e->result;
      return true;
    }
  return false;
}

void eventLoopDestroy(struct EventLoop *loop) {
  if (!loop) return;
  loop->head = NULL;
  loop->tail = NULL;
  loop->count = 0;
}

void eventLoopCleanDone(struct EventLoop *loop) {
  if (!loop) return;
  struct AsyncEvent **pp = &loop->head;
  while (*pp) {
    struct AsyncEvent *e = *pp;
    if (e->state == ASYNC_DONE) {
      *pp = e->next;
      loop->count--;
    } else {
      pp = &e->next;
    }
  }
  loop->tail = NULL;
  for (struct AsyncEvent *e = loop->head; e; e = e->next)
    loop->tail = e;
}

/**
 * @brief Check if there are any pending (non-DONE) events.
 */
bool eventLoopHasPending(struct EventLoop *loop) {
  if (!loop) return false;
  for (struct AsyncEvent *e = loop->head; e; e = e->next)
    if (e->state != ASYNC_DONE) return true;
  return false;
}
