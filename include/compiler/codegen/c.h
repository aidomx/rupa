#pragma once
#if defined(RUPA_PACKAGE_H)

/* Compile an IR module through C and the host C compiler. */
int compileIR(IRModule *ir, const char *output);

#endif
