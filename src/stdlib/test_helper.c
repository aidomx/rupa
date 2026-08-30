#include <rupa.h>

static int g_assert_count = 0;
static int g_assert_failures = 0;

void testHelperReset(void) {
  g_assert_count = 0;
  g_assert_failures = 0;
}

int testHelperFailures(void) { return g_assert_failures; }

/* ==================== assert(condition) ==================== */
static InterpreterResult testAssert(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error) {
  (void)env;
  g_assert_count++;

  if (argc < 1) {
    g_assert_failures++;
    if (error)
      addError(error, (ErrorInfo){.code = "AssertError",
                                  .message = "assert() requires 1 argument",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_ASSERT_FAILED});
    return resultNormal(valueNull());
  }

  if (!valueTruthy(argv[0])) {
    g_assert_failures++;
    if (error)
      addError(error,
               (ErrorInfo){.code = "AssertError",
                           .message = "assertion failed: condition is falsy",
                           .line = 0,
                           .row = 0,
                           .type = ERR_ASSERT_FAILED});
  }
  return resultNormal(valueNull());
}

/* ==================== assertEq(actual, expected) ==================== */
static InterpreterResult testAssertEq(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error) {
  (void)env;
  g_assert_count++;

  if (argc < 2) {
    g_assert_failures++;
    if (error)
      addError(error, (ErrorInfo){.code = "AssertError",
                                  .message = "assertEq() requires 2 arguments",
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_ASSERT_FAILED});
    return resultNormal(valueNull());
  }

  if (!valueEquals(argv[0], argv[1])) {
    g_assert_failures++;
    /* Build a descriptive error message */
    char actual_buf[128] = {0};
    char expected_buf[128] = {0};

    /* Simple type name extraction */
    snprintf(actual_buf, sizeof(actual_buf), "type=%s",
             valueTypeName(argv[0].type));
    snprintf(expected_buf, sizeof(expected_buf), "type=%s",
             valueTypeName(argv[1].type));

    char msg[512];
    snprintf(msg, sizeof(msg), "assertEq failed: got %s, expected %s",
             actual_buf, expected_buf);

    if (error)
      addError(error, (ErrorInfo){.code = "AssertError",
                                  .message = strdup(msg),
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_ASSERT_FAILED});
  }
  return resultNormal(valueNull());
}

/* ==================== assertType(value, typeName) ==================== */
static InterpreterResult testAssertType(int argc, RuntimeValue *argv,
                                        RuntimeEnv *env, Error *error) {
  (void)env;
  g_assert_count++;

  if (argc < 2 || argv[1].type != VALUE_STRING || !argv[1].as.string) {
    g_assert_failures++;
    if (error)
      addError(error,
               (ErrorInfo){.code = "AssertError",
                           .message = "assertType() requires (value, string)",
                           .line = 0,
                           .row = 0,
                           .type = ERR_ASSERT_FAILED});
    return resultNormal(valueNull());
  }

  const char *expected_type = argv[1].as.string;
  const char *actual_type = valueTypeName(argv[0].type);

  if (strcmp(actual_type, expected_type) != 0) {
    g_assert_failures++;
    char msg[256];
    snprintf(msg, sizeof(msg), "assertType failed: got '%s', expected '%s'",
             actual_type, expected_type);
    if (error)
      addError(error, (ErrorInfo){.code = "AssertError",
                                  .message = strdup(msg),
                                  .line = 0,
                                  .row = 0,
                                  .type = ERR_ASSERT_FAILED});
  }
  return resultNormal(valueNull());
}

/* ==================== getFailures() ==================== */
static InterpreterResult testGetFailures(int argc, RuntimeValue *argv,
                                         RuntimeEnv *env, Error *error) {
  (void)argc;
  (void)argv;
  (void)env;
  (void)error;
  return resultNormal(valueNumber(g_assert_failures));
}

/* ==================== Module init ==================== */
void testHelperInit(RuntimeEnv *env) {
  if (!env)
    return;

  semSet(env, "assert", valueNativeFunction("assert", testAssert, 1));
  semSet(env, "assertEq", valueNativeFunction("assertEq", testAssertEq, 2));
  semSet(env, "assertType",
         valueNativeFunction("assertType", testAssertType, 2));
  semSet(env, "getFailures",
         valueNativeFunction("getFailures", testGetFailures, 0));
}
