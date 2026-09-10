#include <rupa.h>

/* HTTP Client — extracted from http.c for modularity.
 * Contains HTTP client methods (GET, POST, PUT, DELETE, PATCH) and module init.
 * Server functions remain in http_server.c. */

/* ==================== http.request(method, url, data) ==================== */
InterpreterResult httpRequest(int argc, RuntimeValue *argv,
                              RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2)
    return resultFlow(FLOW_ERROR,
                      valueString("http.request() expects method and url"));

  if (argv[0].type != VALUE_STRING || argv[1].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR,
                      valueString("http.request() expects string arguments"));

  const char *method = argv[0].as.string;
  const char *url = argv[1].as.string;

  /* Simple HTTP client using system() with curl */
  char cmd[2048];
  char tmpfile[] = "/tmp/rupa_http_response_XXXXXX";
  int fd = mkstemp(tmpfile);

  if (strcmp(method, "GET") == 0) {
    snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s\"", tmpfile, url);
  } else if (strcmp(method, "POST") == 0) {
    const char *data = argc > 2 && argv[2].type == VALUE_STRING
                           ? argv[2].as.string
                           : "";
    snprintf(cmd, sizeof(cmd), "curl -s -X POST -d \"%s\" -o \"%s\" \"%s\"", data,
             tmpfile, url);
  } else if (strcmp(method, "PUT") == 0) {
    const char *data = argc > 2 && argv[2].type == VALUE_STRING
                           ? argv[2].as.string
                           : "";
    snprintf(cmd, sizeof(cmd), "curl -s -X PUT -d \"%s\" -o \"%s\" \"%s\"", data,
             tmpfile, url);
  } else if (strcmp(method, "DELETE") == 0) {
    snprintf(cmd, sizeof(cmd), "curl -s -X DELETE -o \"%s\" \"%s\"", tmpfile, url);
  } else if (strcmp(method, "PATCH") == 0) {
    const char *data = argc > 2 && argv[2].type == VALUE_STRING
                           ? argv[2].as.string
                           : "";
    snprintf(cmd, sizeof(cmd), "curl -s -X PATCH -d \"%s\" -o \"%s\" \"%s\"", data,
             tmpfile, url);
  } else {
    close(fd);
    unlink(tmpfile);
    return resultFlow(FLOW_ERROR,
                      valueString("Unsupported HTTP method"));
  }

  int ret = system(cmd);
  close(fd);

  if (ret != 0) {
    unlink(tmpfile);
    return resultFlow(FLOW_ERROR, valueString("HTTP request failed"));
  }

  /* Read response */
  FILE *f = fopen(tmpfile, "r");
  if (!f) {
    unlink(tmpfile);
    return resultFlow(FLOW_ERROR, valueString("Failed to read response"));
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = gcmall(fsize + 1);
  fread(content, 1, fsize, f);
  content[fsize] = '\0';
  fclose(f);
  unlink(tmpfile);

  return resultNormal(valueString(content));
}

/* ==================== http.get(url) ==================== */
static InterpreterResult httpGet(int argc, RuntimeValue *argv,
                                RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR, valueString("http.get() expects a URL"));

  RuntimeValue args[] = {valueString("GET"), argv[0]};
  return httpRequest(2, args, env, error);
}

/* ==================== http.post(url, data) ==================== */
static InterpreterResult httpPost(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR,
                      valueString("http.post() expects URL and data"));

  RuntimeValue args[] = {valueString("POST"), argv[0],
                         argc > 1 ? argv[1] : valueString("")};
  return httpRequest(3, args, env, error);
}

/* ==================== http.put(url, data) ==================== */
static InterpreterResult httpPut(int argc, RuntimeValue *argv,
                                 RuntimeEnv *env, Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR,
                      valueString("http.put() expects URL and data"));

  RuntimeValue args[] = {valueString("PUT"), argv[0],
                         argc > 1 ? argv[1] : valueString("")};
  return httpRequest(3, args, env, error);
}

/* ==================== http.delete(url) ==================== */
static InterpreterResult httpDelete(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR, valueString("http.delete() expects a URL"));

  RuntimeValue args[] = {valueString("DELETE"), argv[0]};
  return httpRequest(2, args, env, error);
}

/* ==================== http.patch(url, data) ==================== */
static InterpreterResult httpPatch(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING)
    return resultFlow(FLOW_ERROR,
                      valueString("http.patch() expects URL and data"));

  RuntimeValue args[] = {valueString("PATCH"), argv[0],
                         argc > 1 ? argv[1] : valueString("")};
  return httpRequest(3, args, env, error);
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

InterpreterResult stdHttpInit(Node *node, int id, RuntimeEnv *env,
                              Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;
  struct RuntimeObjectEntry *entries = NULL;

  addEntry(&entries, "server", httpServer, 1);
  addEntry(&entries, "stop", httpStop, 1);
  addEntry(&entries, "request", httpRequest, 2);
  addEntry(&entries, "get", httpGet, 1);
  addEntry(&entries, "post", httpPost, 2);
  addEntry(&entries, "put", httpPut, 2);
  addEntry(&entries, "delete", httpDelete, 1);
  addEntry(&entries, "patch", httpPatch, 2);

  return resultNormal(valueObject(entries));
}
