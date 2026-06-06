#pragma once

#if defined(RUPA_PACKAGE_H)

extern int scanId(Atom *atom);
extern int getIdentifier(const char *content, int *pos);
extern bool nextChar(char c);

#endif
