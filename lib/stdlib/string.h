#pragma once
#if defined(RUPA_PACKAGE_H)

/* Native string methods used by primitive member access. */
InterpreterResult stdStringUpper(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error);
InterpreterResult stdStringLower(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                 Error *error);
InterpreterResult stdStringTrim(int argc, RuntimeValue *argv, RuntimeEnv *env,
                                Error *error);
InterpreterResult stdStringContains(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error);
InterpreterResult stdStringStartsWith(int argc, RuntimeValue *argv,
                                      RuntimeEnv *env, Error *error);
InterpreterResult stdStringEndsWith(int argc, RuntimeValue *argv,
                                    RuntimeEnv *env, Error *error);
InterpreterResult stdStringReplace(int argc, RuntimeValue *argv,
                                   RuntimeEnv *env, Error *error);

#endif
