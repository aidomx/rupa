#include <rupa.h>

/* ==================== Validation helpers ==================== */

static InterpreterResult dtError(Error *error, const char *name, const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"TypeError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  (void)name;
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== datetime.now() ==================== */
/* Returns current Unix timestamp in seconds */

static InterpreterResult dtNow(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)argc;
  (void)argv;
  (void)env;
  (void)error;
  time_t now = time(NULL);
  return resultNormal(valueNumber((int)now));
}

/* ==================== datetime.nowMs() ==================== */
/* Returns current Unix timestamp in milliseconds */

static InterpreterResult dtNowMs(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)argc;
  (void)argv;
  (void)env;
  (void)error;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  long long ms = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  return resultNormal(valueNumber((int)ms));
}

/* ==================== datetime.format(timestamp, fmt?) ==================== */
/* Format a Unix timestamp to string. Default format: "YYYY-MM-DD HH:MM:SS" */

static InterpreterResult dtFormat(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return dtError(error, "format", "datetime.format() expects a timestamp");

  time_t ts;
  if (argv[0].type == VALUE_NUMBER)
    ts = (time_t)argv[0].as.number;
  else if (argv[0].type == VALUE_DECIMAL)
    ts = (time_t)argv[0].as.decimal;
  else
    return dtError(error, "format", "datetime.format() expects a number");

  const char *fmt = "%Y-%m-%d %H:%M:%S";
  if (argc >= 2 && argv[1].type == VALUE_STRING && argv[1].as.string) fmt = argv[1].as.string;

  struct tm *tm_info = localtime(&ts);
  if (!tm_info) return dtError(error, "format", "datetime.format() invalid timestamp");

  char buf[256];
  strftime(buf, sizeof(buf), fmt, tm_info);

  return resultNormal(valueString(gcstrdup(buf)));
}

/* ==================== datetime.parse(str, fmt?) ==================== */
/* Parse a date string to Unix timestamp */

static InterpreterResult dtParse(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || !argv || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return dtError(error, "parse", "datetime.parse() expects a string");

  const char *str = argv[0].as.string;
  const char *fmt = "%Y-%m-%d %H:%M:%S";
  if (argc >= 2 && argv[1].type == VALUE_STRING && argv[1].as.string) fmt = argv[1].as.string;

  struct tm tm_info = {0};
  char *end = gcmall(sizeof(char));
  end = gcstrdup(str);

  strftime(end, sizeof(end), fmt, &tm_info);
  if (end == NULL) return dtError(error, "parse", "datetime.parse() failed to parse string");

  time_t ts = mktime(&tm_info);
  if (ts == -1) return dtError(error, "parse", "datetime.parse() invalid date");

  return resultNormal(valueNumber((int)ts));
}

/* ==================== datetime.diff(ts1, ts2) ==================== */
/* Returns difference in seconds between two timestamps */

static InterpreterResult dtDiff(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return dtError(error, "diff", "datetime.diff() expects two timestamps");

  time_t ts1, ts2;
  if (argv[0].type == VALUE_NUMBER)
    ts1 = (time_t)argv[0].as.number;
  else
    return dtError(error, "diff", "datetime.diff() first arg must be number");

  if (argv[1].type == VALUE_NUMBER)
    ts2 = (time_t)argv[1].as.number;
  else
    return dtError(error, "diff", "datetime.diff() second arg must be number");

  return resultNormal(valueNumber((int)difftime(ts2, ts1)));
}

/* ==================== datetime.add(timestamp, seconds) ==================== */
/* Add seconds to a timestamp */

static InterpreterResult dtAdd(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return dtError(error, "add", "datetime.add() expects timestamp and seconds");

  time_t ts;
  if (argv[0].type == VALUE_NUMBER)
    ts = (time_t)argv[0].as.number;
  else
    return dtError(error, "add", "datetime.add() first arg must be number");

  int secs;
  if (argv[1].type == VALUE_NUMBER)
    secs = argv[1].as.number;
  else if (argv[1].type == VALUE_DECIMAL)
    secs = (int)argv[1].as.decimal;
  else
    return dtError(error, "add", "datetime.add() second arg must be number");

  return resultNormal(valueNumber((int)(ts + secs)));
}

/* ==================== datetime.year(ts?) ==================== */
/* Get year from timestamp (default: now) */

static InterpreterResult dtYear(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "year", "datetime.year() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_year + 1900));
}

/* ==================== datetime.month(ts?) ==================== */

static InterpreterResult dtMonth(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "month", "datetime.month() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_mon + 1));
}

/* ==================== datetime.day(ts?) ==================== */

static InterpreterResult dtDay(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "day", "datetime.day() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_mday));
}

/* ==================== datetime.hour(ts?) ==================== */

static InterpreterResult dtHour(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "hour", "datetime.hour() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_hour));
}

/* ==================== datetime.minute(ts?) ==================== */

static InterpreterResult dtMinute(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "minute", "datetime.minute() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_min));
}

/* ==================== datetime.second(ts?) ==================== */

static InterpreterResult dtSecond(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  time_t ts = time(NULL);
  if (argc >= 1) {
    if (argv[0].type == VALUE_NUMBER)
      ts = (time_t)argv[0].as.number;
    else
      return dtError(error, "second", "datetime.second() expects a number");
  }
  struct tm *tm_info = localtime(&ts);
  return resultNormal(valueNumber(tm_info->tm_sec));
}

/* ==================== Module init ==================== */

InterpreterResult stdDatetimeInit(Node *node, int id, RuntimeEnv *env, Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  RuntimeValue mod = valueObject(NULL);
  RuntimeValue fn;

  fn = valueNativeFunction("now", dtNow, 0);
  valueObjectSet(&mod, "now", fn);

  fn = valueNativeFunction("nowMs", dtNowMs, 0);
  valueObjectSet(&mod, "nowMs", fn);

  fn = valueNativeFunction("format", dtFormat, 1);
  valueObjectSet(&mod, "format", fn);

  fn = valueNativeFunction("parse", dtParse, 1);
  valueObjectSet(&mod, "parse", fn);

  fn = valueNativeFunction("diff", dtDiff, 2);
  valueObjectSet(&mod, "diff", fn);

  fn = valueNativeFunction("add", dtAdd, 2);
  valueObjectSet(&mod, "add", fn);

  fn = valueNativeFunction("year", dtYear, 0);
  valueObjectSet(&mod, "year", fn);

  fn = valueNativeFunction("month", dtMonth, 0);
  valueObjectSet(&mod, "month", fn);

  fn = valueNativeFunction("day", dtDay, 0);
  valueObjectSet(&mod, "day", fn);

  fn = valueNativeFunction("hour", dtHour, 0);
  valueObjectSet(&mod, "hour", fn);

  fn = valueNativeFunction("minute", dtMinute, 0);
  valueObjectSet(&mod, "minute", fn);

  fn = valueNativeFunction("second", dtSecond, 0);
  valueObjectSet(&mod, "second", fn);

  return resultNormal(mod);
}
