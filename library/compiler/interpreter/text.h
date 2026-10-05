#pragma once
#if defined(RUPA_PACKAGE_H)

/* Wrapper publik untuk textOf() di expression/binary.c — dipakai mesin
 * IR (execute.c) agar concat `+` men-stringify object/array sejajar
 * persis dengan interpreter (menghilangkan duplikasi format manual). */
char *valueTextOf(RuntimeValue value);

/* Terapkan operator aritmetika/konkatenasi pada dua value — semantik
 * persis interpretBinary. Dipakai interpretUpdate untuk compound
 * assignment (+=, -=, *=, /=, %=). *ok=false bila op tidak berlaku. */
RuntimeValue valueBinaryApply(const char *op, RuntimeValue left,
                              RuntimeValue right, bool *ok);

/* Format node type annotation (NODE_IDENTIFIER / NODE_ARRAY_TYPE) menjadi
 * string "number", "Circuit[]", dst. Boleh NULL untuk type id -1. */
bool formatAstTypeName(Node *node, int typeId, char *buffer, size_t capacity);

#endif
