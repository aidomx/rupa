#pragma once

#if defined(RUPA_PACKAGE_H)

/* prompt.c — UI */
extern void welcomeMessage(void);
extern void help(bool prepend);
extern void showModuleHelp(void);
extern void showTestHelp(void);
extern void showFmtHelp(void);
extern void version(void);

/* compile class */
extern int goCompile(const char *paths[], int length);

/* spec.c — .spec config generate + encrypt */
extern int specCommand(const char *paths[], int length);
extern bool specPromptPassword(const char *prompt, char *buf, int n);
extern char *specDecryptText(const char *algo, const char *pass, const char *checkHex,
                             const char *hex);

/* serve.c — serve.rp runner dengan hot reload (mode dev) */
extern int serveRun(const char *path, int portFallback, bool detail);

/* runner.c — file & code execution */
extern int run(const char *paths[], int length);
extern void execute(const char *code);

/* test.c — syntax, exec, and REPL tests */
extern void test(const char *paths[], int length);
extern void testAst(const char *paths[], int length);
extern void testIR(const char *paths[], int length);
extern void testIRExec(const char *paths[], int length);
extern void testExec(const char *paths[], int length);
extern void testFmt(const char *paths[], int length);
extern void testRepl(const char *paths[], int length);
extern int testDispatch(const char *args[], int length);

/* formatter.c — code formatting */
extern int formatFile(const char *path);
extern int formatString(const char *source);
extern int formatStdin(void);

#endif
