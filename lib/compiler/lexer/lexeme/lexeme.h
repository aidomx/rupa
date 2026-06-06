#pragma once

#include "scan_number.h"
#include "scan_string.h"
#include "scan_symbol.h"
#include "token.h"

#if defined(RUPA_PACKAGE_H)

extern int lexeme(Atom *atom, const char *content, ExceptType except);

#endif
