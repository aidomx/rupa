#pragma once

#if defined(RUPA_PACKAGE_H)

/* prompt.c — UI */
extern void welcomeMessage(void);
extern void help(bool prepend);
extern void version(void);

/* runner.c — file & code execution */
extern void run(const char *paths[], int length);
extern void execute(const char *code);

/* test.c — syntax, exec, and REPL tests */
extern void test(const char *paths[], int length);
extern void testAst(const char *paths[], int length);
extern void testExec(const char *paths[], int length);
extern void testRepl(const char *paths[], int length);

#endif
