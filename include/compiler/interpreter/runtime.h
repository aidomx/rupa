#pragma once

#if defined(RUPA_PACKAGE_H)

RuntimeValue valueNull(void);
RuntimeValue valueNumber(long long value);
RuntimeValue valueDecimal(double value);
RuntimeValue valueBoolean(bool value);
RuntimeValue valueString(const char *value);
RuntimeValue valueArray(RuntimeValue *items, int length);
RuntimeValue valueFunction(RuntimeFunction *function);
RuntimeValue valueObject(struct RuntimeObjectEntry *entries);
RuntimeValue valueNativeFunction(const char *name, NativeFn func, int paramCount);
/* Write-back binding utk member call native (mis. o.set(...)): nama
 * binding receiver di env asal — setelah call, native menulis kembali
 * receiver ke binding ini (object disalin by value saat bind). */
RuntimeValue valuePtr(void *ptr);
/* Color print (design/next_print.txt): value warna dibungkus VALUE_OBJECT
 * bertag "__color" + "__rgb" (24-bit 0xRRGGBB) — bukan ValueType baru. */
RuntimeValue valueColor(long long rgb);
bool valueColorOf(RuntimeValue value, long long *rgbOut);
bool valueObjectGet(RuntimeValue obj, const char *key, RuntimeValue *out);
bool valueObjectSet(RuntimeValue *obj, const char *key, RuntimeValue value);
void valuePrint(RuntimeValue value);
void valuePrintInterp(RuntimeValue value, struct RuntimeEnv *env, struct Error *error);
/* Render-to-buffer untuk print engine (print_format.c): pola 1/2/4
 * merender ke buffer, output akhir ditulis printRenderArgs(). */
void valuePrintTo(char **buf, size_t *len, size_t *cap, RuntimeValue value);
void printBufAppend(char **buf, size_t *len, size_t *cap, const char *text);
void printStringInterpTo(char **buf, size_t *len, size_t *cap, const char *str,
                         struct RuntimeEnv *env, struct Error *error);
bool valueSpecRejected(RuntimeValue value, struct Error *error);
bool valueTruthy(RuntimeValue value);
bool valueEquals(RuntimeValue left, RuntimeValue right);
/* Variable management is provided by the semantic layer.
 * See lib/compiler/semantic/symbol.h for semCreateEnv, semSet, semGet,
 * semDeclare, semType. */
InterpreterResult resultNormal(RuntimeValue value);
InterpreterResult resultFlow(InterpreterFlow flow, RuntimeValue value);
const char *valueTypeName(ValueType type);

#endif
