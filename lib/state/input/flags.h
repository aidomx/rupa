#pragma once
#if defined(RUPA_PACKAGE_H)

extern Flags *createFlags(size_t capacity);
void clearFlags(Flags *flags);
extern void resetFlags(Flags *flags);

#endif
