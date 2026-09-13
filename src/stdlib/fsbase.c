#include <rupa.h>

/* Primitives filesystem: read/write/append/size/isDir/exists.
 * Level tinggi (readLines, copy, move, …) ditulis dalam bahasa Rupa
 * di stdlib/fs yang mem-fasade modul native ini (rupa.fsbase). */

static InterpreterResult fsError(Error *error, const char *code,
                                 const char *msg) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)code,
                                .message = (char *)msg,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== fsbase.read(path) ==================== */
static InterpreterResult fsRead(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return fsError(error, "TypeError", "fsbase.read() expects a string path");

  FILE *fp = fopen(argv[0].as.string, "rb");
  if (!fp)
    return fsError(error, "IOError", "cannot open file for reading");

  char buf[8192];
  size_t total = 0, cap = 16384;
  char *out = malloc(cap);
  if (!out) {
    fclose(fp);
    return fsError(error, "IOError", "out of memory");
  }
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
    if (total + n + 1 > cap) {
      cap = (total + n + 1) * 2;
      char *tmp = realloc(out, cap);
      if (!tmp) {
        free(out);
        fclose(fp);
        return fsError(error, "IOError", "out of memory");
      }
      out = tmp;
    }
    memcpy(out + total, buf, n);
    total += n;
  }
  fclose(fp);
  out[total] = '\0';

  RuntimeValue result = valueString(out);
  free(out);
  return resultNormal(result);
}

/* ==================== fsbase.write(path, content) ==================== */
static InterpreterResult fsWrite(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING || !argv[0].as.string ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return fsError(error, "TypeError",
                   "fsbase.write() expects (path, content) strings");

  FILE *fp = fopen(argv[0].as.string, "wb");
  if (!fp)
    return fsError(error, "IOError", "cannot open file for writing");
  size_t len = strlen(argv[1].as.string);
  bool ok = fwrite(argv[1].as.string, 1, len, fp) == len;
  fclose(fp);
  if (!ok)
    return fsError(error, "IOError", "write failed");
  return resultNormal(valueBoolean(true));
}

/* ==================== fsbase.append(path, content) ==================== */
static InterpreterResult fsAppend(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  if (argc < 2 || argv[0].type != VALUE_STRING || !argv[0].as.string ||
      argv[1].type != VALUE_STRING || !argv[1].as.string)
    return fsError(error, "TypeError",
                   "fsbase.append() expects (path, content) strings");

  FILE *fp = fopen(argv[0].as.string, "ab");
  if (!fp)
    return fsError(error, "IOError", "cannot open file for appending");
  size_t len = strlen(argv[1].as.string);
  bool ok = fwrite(argv[1].as.string, 1, len, fp) == len;
  fclose(fp);
  if (!ok)
    return fsError(error, "IOError", "append failed");
  return resultNormal(valueBoolean(true));
}

/* ==================== fsbase.size(path) ==================== */
static InterpreterResult fsSize(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return fsError(error, "TypeError", "fsbase.size() expects a string path");
  struct stat st;
  if (stat(argv[0].as.string, &st) != 0)
    return resultNormal(valueNumber(-1));
  return resultNormal(valueNumber((int)st.st_size));
}

/* ==================== fsbase.isDir(path) ==================== */
static InterpreterResult fsIsDir(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return fsError(error, "TypeError", "fsbase.isDir() expects a string path");
  struct stat st;
  if (stat(argv[0].as.string, &st) != 0)
    return resultNormal(valueBoolean(false));
  return resultNormal(valueBoolean(S_ISDIR(st.st_mode)));
}

/* ==================== fsbase.exists(path) ==================== */
static InterpreterResult fsExists(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return fsError(error, "TypeError", "fsbase.exists() expects a string path");
  struct stat st;
  return resultNormal(valueBoolean(stat(argv[0].as.string, &st) == 0));
}

/* ==================== fsbase.remove(path) ==================== */
static InterpreterResult fsRemove(int argc, RuntimeValue *argv,
                                  RuntimeEnv *env, Error *error) {
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return fsError(error, "TypeError", "fsbase.remove() expects a string path");
  if (remove(argv[0].as.string) != 0)
    return fsError(error, "IOError", "cannot remove file");
  return resultNormal(valueBoolean(true));
}

/* ==================== Module init ==================== */
static void addEntry(struct RuntimeObjectEntry **head, const char *name,
                     NativeFn fn, int paramCount) {
  struct RuntimeObjectEntry *e = calloc(1, sizeof(*e));
  if (!e)
    return;
  e->key = strdup(name);
  e->value = valueNativeFunction(name, fn, paramCount);
  e->next = *head;
  *head = e;
}

InterpreterResult stdFsbaseInit(Node *node, int id, RuntimeEnv *env,
                                Error *error) {
  struct RuntimeObjectEntry *entries = NULL;

  addEntry(&entries, "read", fsRead, 1);
  addEntry(&entries, "write", fsWrite, 2);
  addEntry(&entries, "append", fsAppend, 2);
  addEntry(&entries, "size", fsSize, 1);
  addEntry(&entries, "isDir", fsIsDir, 1);
  addEntry(&entries, "exists", fsExists, 1);
  addEntry(&entries, "remove", fsRemove, 1);

  return resultNormal(valueObject(entries));
}
