#pragma once
#if defined(RUPA_PACKAGE_H)

/* Initialize all standard modules and register them in the environment */
void stdlibInit(RuntimeEnv *env);

/* Get a standard module by name */
bool stdlibGetModule(const char *name, RuntimeValue *out);

#endif
