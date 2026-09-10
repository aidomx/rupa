#include <rupa.h>

/* HTTP Server — thread-safe design.
 * Server thread: accept, recv, enqueue request, wait for response, send, close.
 * Main thread: dequeue request, call handler (interpretNode), set response.
 *
 * This avoids calling interpretNode from the server thread, which is not
 * thread-safe due to shared global state (event loop, symbol table, etc.). */

/* Server state (declared in rupa_modules.h, defined here) */
ServerEntry serverTable[MAX_SERVERS];
int serverCount = 0;
pthread_mutex_t serverMutex = PTHREAD_MUTEX_INITIALIZER;

/* Keep-alive: main thread runs event loop while server is running */
static pthread_mutex_t keepAliveMutex = PTHREAD_MUTEX_INITIALIZER;
static bool keepAliveRunning = false;

/* ==================== HTTP Request/Response types ==================== */

typedef struct {
  char method[16];
  char path[256];
  char headers[MAX_REQUEST_SIZE];
  char body[MAX_REQUEST_SIZE];
  int body_len;
} HttpRequest;

typedef struct {
  int status;
  char status_text[64];
  char headers_buf[MAX_RESPONSE_SIZE];
  int headers_len;
  char body[MAX_RESPONSE_SIZE];
  int body_len;
} HttpResponse;

/* ==================== Request Queue ==================== */
#define MAX_PENDING 64

typedef struct {
  int client_fd;                  /* Client socket fd */
  int server_id;                  /* Which server owns this request */
  char request[MAX_REQUEST_SIZE]; /* Raw HTTP request data */
  int request_len;                /* Length of request data */

  /* Response filled by handler (main thread) */
  HttpResponse response;
  bool response_ready;            /* Main thread has filled the response */
  bool in_use;                    /* Slot is occupied */

  pthread_mutex_t mutex;
  pthread_cond_t response_cond;   /* Server thread waits on this */
} PendingRequest;

static PendingRequest pendingQueue[MAX_PENDING];
static pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t requestReadyCond = PTHREAD_COND_INITIALIZER;
static int pendingCount = 0;

/* Enqueue a request from the server thread */
static PendingRequest *enqueueRequest(int client_fd, int server_id,
                                      const char *data, int len) {
  pthread_mutex_lock(&queueMutex);
  for (int i = 0; i < MAX_PENDING; i++) {
    if (!pendingQueue[i].in_use) {
      PendingRequest *pr = &pendingQueue[i];
      pr->in_use = true;
      pr->client_fd = client_fd;
      pr->server_id = server_id;
      pr->response_ready = false;
      pr->request_len = len < MAX_REQUEST_SIZE ? len : MAX_REQUEST_SIZE - 1;
      memcpy(pr->request, data, pr->request_len);
      pr->request[pr->request_len] = '\0';
      memset(&pr->response, 0, sizeof(HttpResponse));
      pr->response.status = 200;
      strcpy(pr->response.status_text, "OK");
      strcpy(pr->response.headers_buf, "Content-Type: text/plain\r\n");
      pr->response.headers_len = (int)strlen(pr->response.headers_buf);
      pendingCount++;
      pthread_cond_signal(&requestReadyCond);
      pthread_mutex_unlock(&queueMutex);
      return pr;
    }
  }
  pthread_mutex_unlock(&queueMutex);
  return NULL;
}

static void parseRequest(const char *raw, HttpRequest *req) {
  memset(req, 0, sizeof(HttpRequest));

  const char *p = raw;
  int i = 0;
  while (*p && *p != ' ' && i < 15)
    req->method[i++] = *p++;
  req->method[i] = '\0';

  if (*p == ' ') p++;

  i = 0;
  while (*p && *p != ' ' && *p != '\r' && i < 255)
    req->path[i++] = *p++;
  req->path[i] = '\0';

  const char *header_start = strstr(raw, "\r\n");
  const char *body_start = strstr(raw, "\r\n\r\n");

  if (header_start) {
    header_start += 2;
    if (body_start) {
      int header_len = (int)(body_start - header_start);
      if (header_len >= MAX_REQUEST_SIZE) header_len = MAX_REQUEST_SIZE - 1;
      memcpy(req->headers, header_start, header_len);
      req->headers[header_len] = '\0';
      body_start += 4;
      req->body_len = (int)strlen(body_start);
      if (req->body_len >= MAX_REQUEST_SIZE) req->body_len = MAX_REQUEST_SIZE - 1;
      memcpy(req->body, body_start, req->body_len);
      req->body[req->body_len] = '\0';
    }
  }
}

/* ==================== res.setHeader(key, value) ==================== */
static InterpreterResult resSetHeader(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2 || argv[0].type != VALUE_STRING || argv[1].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR,
                      valueString("res.setHeader() expects two strings"));

  /* Find the pending request being processed */
  pthread_mutex_lock(&queueMutex);
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingQueue[i].in_use && !pendingQueue[i].response_ready) {
      HttpResponse *res = &pendingQueue[i].response;
      int written = snprintf(res->headers_buf + res->headers_len,
                             sizeof(res->headers_buf) - res->headers_len,
                             "%s: %s\r\n", argv[0].as.string, argv[1].as.string);
      res->headers_len += written;
      pthread_mutex_unlock(&queueMutex);
      return resultNormal(valueNull());
    }
  }
  pthread_mutex_unlock(&queueMutex);
  return resultFlow(FLOW_ERROR, valueString("No active response"));
}

/* ==================== res.json(obj) ==================== */
static InterpreterResult resJson(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1)
    return resultFlow(FLOW_ERROR, valueString("res.json() expects a value"));

  pthread_mutex_lock(&queueMutex);
  for (int i = 0; i < MAX_PENDING; i++) {
    if (pendingQueue[i].in_use && !pendingQueue[i].response_ready) {
      HttpResponse *res = &pendingQueue[i].response;
      res->headers_len = 0;
      int w = snprintf(res->headers_buf, sizeof(res->headers_buf),
                       "Content-Type: application/json\r\n");
      res->headers_len = w;

      if (argv[0].type == VALUE_STRING && argv[0].as.string) {
        res->body_len = snprintf(res->body, sizeof(res->body), "\"%s\"",
                                 argv[0].as.string);
      } else if (argv[0].type == VALUE_NUMBER) {
        res->body_len = snprintf(res->body, sizeof(res->body), "%d",
                                 argv[0].as.number);
      } else if (argv[0].type == VALUE_BOOLEAN) {
        res->body_len = snprintf(res->body, sizeof(res->body), "%s",
                                 argv[0].as.boolean ? "true" : "false");
      } else if (argv[0].type == VALUE_NULL) {
        res->body_len = snprintf(res->body, sizeof(res->body), "null");
      } else {
        /* Use json.stringify to serialize any value */
        InterpreterResult jr = jsonStringify(1, argv, NULL, NULL);
        if (jr.flow == FLOW_NORMAL && jr.value.type == VALUE_STRING &&
            jr.value.as.string) {
          res->body_len = (int)strlen(jr.value.as.string);
          if (res->body_len >= MAX_RESPONSE_SIZE)
            res->body_len = MAX_RESPONSE_SIZE - 1;
          memcpy(res->body, jr.value.as.string, res->body_len);
          res->body[res->body_len] = '\0';
        }
      }
      pthread_mutex_unlock(&queueMutex);
      return resultNormal(valueNull());
    }
  }
  pthread_mutex_unlock(&queueMutex);
  return resultFlow(FLOW_ERROR, valueString("No active response"));
}

/* ==================== Build response string ==================== */
static int buildResponseString(HttpResponse *res, char *buf, int bufsize) {
  int header_len = res->headers_len;
  if (header_len > 0 && res->headers_buf[header_len - 2] == '\r')
    header_len -= 2;

  int len = snprintf(buf, bufsize,
                     "HTTP/1.1 %d %s\r\n"
                     "%.*s\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     res->status, res->status_text,
                     header_len, res->headers_buf,
                     res->body_len);

  if (len + res->body_len >= bufsize)
    res->body_len = bufsize - len - 1;

  memcpy(buf + len, res->body, res->body_len);
  len += res->body_len;
  buf[len] = '\0';
  return len;
}

/* ==================== Process a pending request (called from main thread) ==================== */
static void processRequest(PendingRequest *pr) {
  ServerEntry *server = &serverTable[pr->server_id];

  HttpRequest req;
  parseRequest(pr->request, &req);

  HttpResponse *res = &pr->response;

  if (server->has_handler) {
    /* Build request object for Rupa */
    struct RuntimeObjectEntry *req_entries = NULL;
    struct RuntimeObjectEntry **req_tail = &req_entries;

    struct RuntimeObjectEntry *e;

    e = calloc(1, sizeof(*e));
    e->key = strdup("method");
    e->value = valueString(req.method);
    *req_tail = e;
    req_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("path");
    e->value = valueString(req.path);
    *req_tail = e;
    req_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("body");
    e->value = valueString(req.body);
    *req_tail = e;
    req_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("headers");
    e->value = valueString(req.headers);
    *req_tail = e;
    req_tail = &e->next;

    RuntimeValue req_val = valueObject(req_entries);

    /* Build response object with setHeader and json methods */
    struct RuntimeObjectEntry *res_entries = NULL;
    struct RuntimeObjectEntry **res_tail = &res_entries;

    e = calloc(1, sizeof(*e));
    e->key = strdup("status");
    e->value = valueNumber(res->status);
    *res_tail = e;
    res_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("body");
    e->value = valueString(res->body);
    *res_tail = e;
    res_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("setHeader");
    e->value = valueNativeFunction("setHeader", resSetHeader, 2);
    *res_tail = e;
    res_tail = &e->next;

    e = calloc(1, sizeof(*e));
    e->key = strdup("json");
    e->value = valueNativeFunction("json", resJson, 1);
    *res_tail = e;
    res_tail = &e->next;

    RuntimeValue res_val = valueObject(res_entries);

    /* Call the handler — safe to call interpretNode from main thread */
    InterpreterResult result;
    if (server->handler.type == VALUE_NATIVE_FUNCTION &&
        server->handler.as.nativeFunc) {
      RuntimeValue args[] = {req_val, res_val};
      result = server->handler.as.nativeFunc->func(2, args, NULL, NULL);
    } else if (server->handler.type == VALUE_FUNCTION &&
               server->handler.as.function) {
      RuntimeFunction *fn = server->handler.as.function;
      RuntimeEnv *local = semCreateEnv(fn->closure);
      if (local) {
        for (int pi = 0; pi < fn->paramLength && pi < 4; pi++) {
          const char *pname = NULL;
          int idx = fn->params[pi];
          if (idx >= 0 && idx < fn->node->length) {
            AstNode *pn = &fn->node->ast[idx];
            if (pn->type == NODE_IDENTIFIER)
              pname = pn->identifier.name;
            else if (pn->type == NODE_LITERAL_ID)
              pname = pn->string.value;
          }
          if (pname) {
            RuntimeValue pv = (pi == 0) ? req_val : res_val;
            semSet(local, pname, pv);
          }
        }
        result = interpretNode(fn->node, fn->body, local, NULL);
        if (result.flow == FLOW_RETURN)
          result = resultNormal(result.value);
      } else {
        result = resultNormal(valueNull());
      }
    } else {
      result = resultNormal(valueNull());
    }

    /* Update response from handler result.
     * Native calls (res.setHeader, res.json) modify pr->response directly.
     * The handler's return value is a backup — update body/status from it
     * only if the handler returned an object with those fields. */
    if (result.flow == FLOW_NORMAL && result.value.type == VALUE_OBJECT) {
      RuntimeValue status_val;
      if (valueObjectGet(result.value, "status", &status_val) &&
          status_val.type == VALUE_NUMBER) {
        res->status = status_val.as.number;
        switch (res->status) {
          case 200: strcpy(res->status_text, "OK"); break;
          case 201: strcpy(res->status_text, "Created"); break;
          case 400: strcpy(res->status_text, "Bad Request"); break;
          case 404: strcpy(res->status_text, "Not Found"); break;
          case 500: strcpy(res->status_text, "Internal Server Error"); break;
          default: strcpy(res->status_text, "OK"); break;
        }
      }

      /* Only override body if the handler explicitly set it
       * (i.e. the body differs from the empty default). */
      RuntimeValue body_val;
      if (valueObjectGet(result.value, "body", &body_val) &&
          body_val.type == VALUE_STRING && body_val.as.string &&
          body_val.as.string[0] != '\0') {
        res->body_len = (int)strlen(body_val.as.string);
        if (res->body_len >= MAX_RESPONSE_SIZE)
          res->body_len = MAX_RESPONSE_SIZE - 1;
        memcpy(res->body, body_val.as.string, res->body_len);
        res->body[res->body_len] = '\0';
      }
    }
  } else {
    const char *msg = "Hello from Rupa HTTP Server!";
    res->body_len = (int)strlen(msg);
    strcpy(res->body, msg);
  }
}

/* ==================== Server thread ==================== */
static void *serverThread(void *arg) {
  int id = *(int *)arg;
  ServerEntry *server = &serverTable[id];

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(server->port);

  server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server->server_fd < 0) {
    server->running = false;
    return NULL;
  }

  int opt = 1;
  setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(server->server_fd);
    server->running = false;
    return NULL;
  }

  if (listen(server->server_fd, 10) < 0) {
    close(server->server_fd);
    server->running = false;
    return NULL;
  }

  while (server->running) {
    int client_fd = accept(server->server_fd, NULL, NULL);
    if (client_fd < 0) break;

    /* Read request */
    char buffer[MAX_REQUEST_SIZE];
    int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
      close(client_fd);
      continue;
    }
    buffer[n] = '\0';

    /* Enqueue request for main thread */
    PendingRequest *pr = enqueueRequest(client_fd, id, buffer, n);
    if (!pr) {
      /* Queue full — send 503 */
      const char *err = "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Length: 0\r\nConnection: close\r\n\r\n";
      send(client_fd, err, (int)strlen(err), 0);
      close(client_fd);
      continue;
    }

    /* Wait for main thread to process the response */
    pthread_mutex_lock(&pr->mutex);
    while (!pr->response_ready && server->running)
      pthread_cond_wait(&pr->response_cond, &pr->mutex);
    pthread_mutex_unlock(&pr->mutex);

    /* Send response */
    if (pr->response_ready) {
      char response_buf[MAX_RESPONSE_SIZE];
      int len = buildResponseString(&pr->response, response_buf,
                                    sizeof(response_buf));
      send(client_fd, response_buf, len, 0);
    }

    close(client_fd);

    /* Mark slot as free */
    pthread_mutex_lock(&pr->mutex);
    pr->in_use = false;
    pr->response_ready = false;
    pthread_mutex_unlock(&pr->mutex);
    pthread_mutex_lock(&queueMutex);
    pendingCount--;
    pthread_mutex_unlock(&queueMutex);
  }

  return NULL;
}

/* ==================== Main thread event loop ==================== */
/* Called from httpServer() — processes queued requests until server stops. */
static void httpEventLoop(void) {
  pthread_mutex_lock(&queueMutex);

  while (keepAliveRunning) {
    /* Wait for a request or stop signal (with timeout to check stop) */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1; /* 1 second timeout for stop check */

    while (pendingCount == 0 && keepAliveRunning) {
      int ret = pthread_cond_timedwait(&requestReadyCond, &queueMutex, &ts);
      if (ret == ETIMEDOUT) {
        /* Timeout — re-check keepAliveRunning in outer loop */
        break;
      }
    }

    if (!keepAliveRunning)
      break;
    if (pendingCount == 0)
      continue; /* Timeout with no requests — loop back */

    /* Find a pending request */
    for (int i = 0; i < MAX_PENDING; i++) {
      if (pendingQueue[i].in_use && !pendingQueue[i].response_ready) {
        PendingRequest *pr = &pendingQueue[i];
        pthread_mutex_unlock(&queueMutex);

        /* Process the request (safe — main thread) */
        processRequest(pr);

        /* Signal server thread that response is ready */
        pthread_mutex_lock(&pr->mutex);
        pr->response_ready = true;
        pthread_cond_signal(&pr->response_cond);
        pthread_mutex_unlock(&pr->mutex);

        pthread_mutex_lock(&queueMutex);
        break;
      }
    }
  }

  pthread_mutex_unlock(&queueMutex);
}

/* ==================== http.server(port, handler) ==================== */
InterpreterResult httpServer(int argc, RuntimeValue *argv,
                             RuntimeEnv *env, Error *error) {
  (void)env;
  (void)error;
  if (argc < 1 || argv[0].type != VALUE_NUMBER)
    return resultFlow(FLOW_ERROR,
                      valueString("http.server() expects a port number"));

  int port = argv[0].as.number;
  if (port < 1 || port > 65535)
    return resultFlow(FLOW_ERROR, valueString("Invalid port number"));

  pthread_mutex_lock(&serverMutex);
  if (serverCount >= MAX_SERVERS) {
    pthread_mutex_unlock(&serverMutex);
    return resultFlow(FLOW_ERROR, valueString("Too many servers"));
  }

  int id = serverCount++;
  ServerEntry *server = &serverTable[id];
  server->port = port;
  server->running = true;
  server->has_handler = false;

  if (argc >= 2 && (argv[1].type == VALUE_NATIVE_FUNCTION ||
                    argv[1].type == VALUE_FUNCTION)) {
    server->handler = argv[1];
    server->has_handler = true;
  }

  pthread_mutex_unlock(&serverMutex);

  /* Initialize pending queue slots */
  for (int i = 0; i < MAX_PENDING; i++) {
    memset(&pendingQueue[i], 0, sizeof(PendingRequest));
    pthread_mutex_init(&pendingQueue[i].mutex, NULL);
    pthread_cond_init(&pendingQueue[i].response_cond, NULL);
  }

  if (pthread_create(&server->thread, NULL, serverThread, &id) != 0) {
    return resultFlow(FLOW_ERROR, valueString("Failed to start server thread"));
  }

  /* Run event loop on main thread — processes handler calls */
  pthread_mutex_lock(&keepAliveMutex);
  keepAliveRunning = true;
  pthread_mutex_unlock(&keepAliveMutex);

  httpEventLoop();

  return resultNormal(valueNumber(id));
}

/* ==================== http.stop(handle) ==================== */
InterpreterResult httpStop(int argc, RuntimeValue *argv,
                           RuntimeEnv *env, Error *error) {
  (void)env;
  (void)error;
  if (argc < 1 || argv[0].type != VALUE_NUMBER)
    return resultFlow(FLOW_ERROR,
                      valueString("http.stop() expects a server handle"));

  int id = argv[0].as.number;
  pthread_mutex_lock(&serverMutex);
  if (id < 0 || id >= serverCount) {
    pthread_mutex_unlock(&serverMutex);
    return resultFlow(FLOW_ERROR, valueString("Invalid server handle"));
  }
  ServerEntry *server = &serverTable[id];
  server->running = false;
  pthread_mutex_unlock(&serverMutex);

  /* Signal server thread to stop */
  shutdown(server->server_fd, SHUT_RDWR);
  close(server->server_fd);

  /* Wake up main thread event loop */
  pthread_mutex_lock(&keepAliveMutex);
  keepAliveRunning = false;
  pthread_mutex_unlock(&keepAliveMutex);
  pthread_cond_signal(&requestReadyCond);

  pthread_join(server->thread, NULL);

  /* Cleanup pending queue */
  for (int i = 0; i < MAX_PENDING; i++) {
    pthread_mutex_destroy(&pendingQueue[i].mutex);
    pthread_cond_destroy(&pendingQueue[i].response_cond);
  }

  return resultNormal(valueNull());
}
