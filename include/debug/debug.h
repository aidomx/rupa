#pragma once
#include "debug_type.h"
#if defined(RUPA_PACKAGE_H)

extern Debug debug;

extern void *createDebug(int capacity);
extern void setdebug(Debug *debug);

#endif
