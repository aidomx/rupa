#pragma once

#if defined(RUPA_PACKAGE_H)

RuntimeValue valueNull(void);
RuntimeValue valueNumber(int value);
RuntimeValue valueDecimal(double value);
RuntimeValue valueBoolean(bool value);
RuntimeValue valueString(const char *value);
RuntimeValue valueArray(RuntimeValue *items, int length);
RuntimeValue valueFunction(RuntimeFunction *function);
RuntimeValue valueObject(struct RuntimeObjectEntry *entries);
RuntimeValue valueNativeFunction(const char *name, NativeFn func, int paramCount);
bool valueObjectGet(RuntimeValue obj, const char *key, RuntimeValue *out);
bool valueObjectSet(RuntimeValue *obj, const char *key, RuntimeValue value);
void valuePrint(RuntimeValue value);
bool valueTruthy(RuntimeValue value);
bool valueEquals(RuntimeValue left, RuntimeValue right);
/* Variable management is provided by the semantic layer.
 * See lib/compiler/semantic/symbol.h for semCreateEnv, semSet, semGet,
 * semDeclare, semType. */
InterpreterResult resultNormal(RuntimeValue value);
InterpreterResult resultFlow(InterpreterFlow flow, RuntimeValue value);
const char *valueTypeName(ValueType type);

#endif
