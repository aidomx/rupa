#pragma once
#if defined(RUPA_PACKAGE_H)

bool validateAnnotation(Node *node, int typeId, RuntimeValue value,
                        Error *error);
bool validateTypeName(const char *type, RuntimeValue value, Error *error);

#endif
