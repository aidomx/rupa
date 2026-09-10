#include <rupa.h>
#include <time.h>

extern struct EventLoop *getEventLoop(void);

static long currentTimeMs(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static RuntimeValue makeAsyncHandle(const char *status, RuntimeValue data,
                                    RuntimeValue errVal) {
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

static void callLoader(Node *node, int loaderId, RuntimeEnv *env,
                       Error *error, RuntimeValue thisVal) {
  if (loaderId < 0) return;
  semSet(env, "this", thisVal);
  int callId = createCall(node, loaderId, NULL, 0);
  if (callId >= 0) interpretNode(node, callId, env, error);
}

static void callHandler(Node *node, int handlerId, RuntimeEnv *env,
                        Error *error, RuntimeValue thisVal) {
  if (handlerId < 0 || handlerId >= node->length) return;
  semSet(env, "this", thisVal);
  interpretNode(node, handlerId, env, error);
}

/*
 * Async expression: call loader(AWAIT), evaluate request, call loader(SUCCESS).
 *
 * Flow:
 *   1. Call loader(AWAIT) → "loading..." appears
 *   2. Evaluate request synchronously
 *   3. Check timeout — if exceeded, call loader(ERROR)
 *   4. Call loader(SUCCESS) → data replaces "loading..."
 */
InterpreterResult interpretAsync(Node *node, int id, AstNode *ast,
                                 RuntimeEnv *env, Error *error) {
  if (!node || !ast || ast->type != NODE_ASYNC)
    return resultNormal(valueNull());

  /* Evaluate timeout if present */
  int timeoutMs = -1;
  if (ast->async.timeout >= 0) {
    InterpreterResult tv = interpretNode(node, ast->async.timeout, env, error);
    if (tv.flow != FLOW_NORMAL) return tv;
    if (tv.value.type == VALUE_NUMBER)
      timeoutMs = tv.value.as.number;
    else if (tv.value.type == VALUE_DECIMAL)
      timeoutMs = (int)tv.value.as.decimal;
  } else if (ast->async.timeoutId >= 0) {
    InterpreterResult tv = interpretNode(node, ast->async.timeoutId, env, error);
    if (tv.flow != FLOW_NORMAL) return tv;
    if (tv.value.type == VALUE_NUMBER)
      timeoutMs = tv.value.as.number;
    else if (tv.value.type == VALUE_DECIMAL)
      timeoutMs = (int)tv.value.as.decimal;
  }

  /* Step 1: Call loader with AWAIT → "loading..." */
  RuntimeValue awaitHandle = makeAsyncHandle("AWAIT", valueNull(), valueNull());
  if (ast->async.loaderId >= 0) {
    callLoader(node, ast->async.loaderId, env, error, awaitHandle);
  } else if (ast->async.handler >= 0) {
    callHandler(node, ast->async.handler, env, error, awaitHandle);
  }

  /* Step 2: Record start time, evaluate request */
  long startTime = currentTimeMs();
  InterpreterResult req = interpretNode(node, ast->async.request, env, error);
  long elapsed = currentTimeMs() - startTime;

  /* Step 3: Check timeout */
  if (timeoutMs > 0 && elapsed >= timeoutMs) {
    RuntimeValue timeoutErr = valueString("Request timed out");
    RuntimeValue errHandle = makeAsyncHandle("ERROR", valueNull(), timeoutErr);
    if (ast->async.loaderId >= 0)
      callLoader(node, ast->async.loaderId, env, error, errHandle);
    else if (ast->async.handler >= 0)
      callHandler(node, ast->async.handler, env, error, errHandle);
    return resultNormal(errHandle);
  }

  /* Step 4: Call loader with SUCCESS or ERROR */
  if (req.flow != FLOW_ERROR) {
    RuntimeValue handle = makeAsyncHandle("SUCCESS", req.value, valueNull());
    if (ast->async.loaderId >= 0) {
      callLoader(node, ast->async.loaderId, env, error, handle);
    } else if (ast->async.handler >= 0) {
      callHandler(node, ast->async.handler, env, error, handle);
    }
    return resultNormal(handle);
  }

  RuntimeValue errHandle = makeAsyncHandle("ERROR", valueNull(), req.value);
  if (ast->async.loaderId >= 0)
    callLoader(node, ast->async.loaderId, env, error, errHandle);
  else if (ast->async.handler >= 0)
    callHandler(node, ast->async.handler, env, error, errHandle);
  return resultNormal(errHandle);
}
