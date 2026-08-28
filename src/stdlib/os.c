#include <rupa.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/stat.h>

/* Helper: create error */
static InterpreterResult osError(Error *error, const char *code, const char *msg) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)code, .message = (char *)msg,
                                  .line = 0, .row = 0, .type = ERR_INTERNAL});
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== os.exec(cmd) ==================== */
static InterpreterResult osExec(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.exec() expects a string argument");

  FILE *fp = popen(argv[0].as.string, "r");
  if (!fp) return osError(error, "IOError", "failed to execute command");

  char buf[4096];
  size_t total = 0;
  char *out = NULL;
  while (fgets(buf, sizeof(buf), fp)) {
    size_t len = strlen(buf);
    char *tmp = realloc(out, total + len + 1);
    if (!tmp) { free(out); pclose(fp); return resultFlow(FLOW_ERROR, valueNull()); }
    out = tmp;
    memcpy(out + total, buf, len);
    total += len;
    out[total] = '\0';
  }
  pclose(fp);
  if (!out) out = strdup("");
  RuntimeValue result = valueString(out);
  free(out);
  return resultNormal(result);
}

/* ==================== os.getcwd() ==================== */
static InterpreterResult osGetcwd(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL)
    return osError(error, "IOError", "failed to get current directory");
  return resultNormal(valueString(cwd));
}

/* ==================== os.exit(code) ==================== */
static InterpreterResult osExit(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  int code = 0;
  if (argc >= 1 && argv[0].type == VALUE_NUMBER) code = argv[0].as.number;
  exit(code);
  return resultNormal(valueNull());
}

/* ==================== os.getenv(key) ==================== */
static InterpreterResult osGetenv(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.getenv() expects a string argument");
  const char *val = getenv(argv[0].as.string);
  return resultNormal(val ? valueString(val) : valueNull());
}

/* ==================== os.chdir(path) ==================== */
static InterpreterResult osChdir(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.chdir() expects a string argument");
  if (chdir(argv[0].as.string) != 0)
    return osError(error, "IOError", "failed to change directory");
  return resultNormal(valueBoolean(true));
}

/* ==================== os.mkdir(path) ==================== */
static InterpreterResult osMkdir(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.mkdir() expects a string argument");
  if (mkdir(argv[0].as.string, 0755) != 0)
    return osError(error, "IOError", "failed to create directory");
  return resultNormal(valueBoolean(true));
}

/* ==================== os.remove(path) ==================== */
static InterpreterResult osRemove(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.remove() expects a string argument");
  if (remove(argv[0].as.string) != 0)
    return osError(error, "IOError", "failed to remove file");
  return resultNormal(valueBoolean(true));
}

/* ==================== os.rename(old, new) ==================== */
static InterpreterResult osRename(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                  Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING || argv[1].type != VALUE_STRING)
    return osError(error, "TypeError", "os.rename() expects two string arguments");
  if (rename(argv[0].as.string, argv[1].as.string) != 0)
    return osError(error, "IOError", "failed to rename file");
  return resultNormal(valueBoolean(true));
}

/* ==================== os.listdir(path) ==================== */
static InterpreterResult osListdir(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                   Error *error) {
  const char *path = ".";
  if (argc >= 1 && argv[0].type == VALUE_STRING) path = argv[0].as.string;

  DIR *d = opendir(path);
  if (!d) return osError(error, "IOError", "failed to open directory");

  int capacity = 16, count = 0;
  RuntimeValue *items = calloc(capacity, sizeof(RuntimeValue));
  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if (count >= capacity) {
      capacity *= 2;
      items = realloc(items, capacity * sizeof(RuntimeValue));
    }
    items[count++] = valueString(entry->d_name);
  }
  closedir(d);

  RuntimeValue result = valueArray(items, count);
  free(items);
  return resultNormal(result);
}

/* ==================== os.path.exists(path) ==================== */
static InterpreterResult osPathExists(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.path.exists() expects a string");
  struct stat st;
  bool exists = (stat(argv[0].as.string, &st) == 0);
  return resultNormal(valueBoolean(exists));
}

/* Create os.path object */
static RuntimeValue createPathObject(void) {
  struct RuntimeObjectEntry *entries = NULL;

  struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
  e->key = strdup("exists");
  e->value = valueNativeFunction("exists", osPathExists, 1);
  e->next = entries;
  entries = e;

  return valueObject(entries);
}

/* ==================== os.info(optional key) ==================== */
static InterpreterResult osInfo(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  struct utsname uts;
  if (uname(&uts) != 0)
    return osError(error, "IOError", "failed to get system info");

  /* If no argument, return full info object */
  if (argc < 1 || argv[0].type == VALUE_NULL) {
    struct RuntimeObjectEntry *entries = NULL;

    /* Helper macro to add entry */
    #define ADD_ENTRY(k, v) do { \
      struct RuntimeObjectEntry *_e = calloc(1, sizeof(*_e)); \
      _e->key = strdup(k); _e->value = valueString(v); \
      _e->next = entries; entries = _e; \
    } while(0)

    ADD_ENTRY("sysname", uts.sysname);
    ADD_ENTRY("nodename", uts.nodename);
    ADD_ENTRY("release", uts.release);
    ADD_ENTRY("version", uts.version);
    ADD_ENTRY("machine", uts.machine);

    /* Get username */
    const char *user = getenv("USER");
    if (!user) user = getenv("LOGNAME");
    if (user) ADD_ENTRY("user", user);

    /* Get hostname */
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0)
      ADD_ENTRY("hostname", hostname);

    #undef ADD_ENTRY
    return resultNormal(valueObject(entries));
  }

  /* If string argument, return specific field */
  if (argv[0].type != VALUE_STRING)
    return osError(error, "TypeError", "os.info() expects a string or no argument");

  const char *key = argv[0].as.string;
  if (strcmp(key, "sysname") == 0) return resultNormal(valueString(uts.sysname));
  if (strcmp(key, "nodename") == 0) return resultNormal(valueString(uts.nodename));
  if (strcmp(key, "release") == 0) return resultNormal(valueString(uts.release));
  if (strcmp(key, "version") == 0) return resultNormal(valueString(uts.version));
  if (strcmp(key, "machine") == 0) return resultNormal(valueString(uts.machine));
  if (strcmp(key, "user") == 0) {
    const char *u = getenv("USER");
    if (!u) u = getenv("LOGNAME");
    return resultNormal(u ? valueString(u) : valueNull());
  }
  if (strcmp(key, "hostname") == 0) {
    char h[256];
    if (gethostname(h, sizeof(h)) == 0) return resultNormal(valueString(h));
    return resultNormal(valueNull());
  }

  return resultNormal(valueNull());
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

InterpreterResult stdOsInit(Node *node, int id, RuntimeEnv *env, Error *error) {
  struct RuntimeObjectEntry *entries = NULL;

  addEntry(&entries, "exec", osExec, 1);
  addEntry(&entries, "getcwd", osGetcwd, 0);
  addEntry(&entries, "exit", osExit, 1);
  addEntry(&entries, "getenv", osGetenv, 1);
  addEntry(&entries, "chdir", osChdir, 1);
  addEntry(&entries, "mkdir", osMkdir, 1);
  addEntry(&entries, "remove", osRemove, 1);
  addEntry(&entries, "rename", osRename, 2);
  addEntry(&entries, "listdir", osListdir, 1);
  addEntry(&entries, "info", osInfo, 1);

  /* Add path sub-object */
  struct RuntimeObjectEntry *path_entry = calloc(1, sizeof(*path_entry));
  path_entry->key = strdup("path");
  path_entry->value = createPathObject();
  path_entry->next = entries;
  entries = path_entry;

  return resultNormal(valueObject(entries));
}
