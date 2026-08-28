#pragma once

#if defined(RUPA_PACKAGE_H)

extern void welcomeMessage(void);
extern void help(bool prepend);
extern void run(const char *paths[], int length);
extern void execute(const char *code);
extern void test(const char *paths[], int length);
extern void version(void);

#endif
