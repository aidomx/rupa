#pragma once
#if defined(RUPA_PACKAGE_H)

bool validateAnnotation(Node *node, int typeId, RuntimeValue value,
                        Error *error);
bool validateTypeName(const char *type, RuntimeValue value, Error *error);

/* Kontrak type permanen: validasi value terhadap type yang dideklarasikan
 * pada binding variable (semType) — berlaku untuk SEMUA assignment,
 * termasuk yang tanpa anotasi. Handle pin family (VALUE_PTR) lewat view
 * check via provenance sizeof pada node value. Return false + error
 * bila melanggar. */
bool validateDeclaredType(Node *node, int valueId, RuntimeEnv *env,
                          const char *name, RuntimeValue value, Error *error);

#endif
