#ifndef RUPA_PROMPT_TEST_INTERNAL_H
#define RUPA_PROMPT_TEST_INTERNAL_H

#include <rupa.h>

/*
 * test_internal.h — kontrak internal antar-unit test (rules.md: modular,
 * jangan membengkak). test.c dipecah menjadi unit kecil:
 *
 *   test_common.c    — helper lintas runner: testPrintSource, testLexParse
 *   test_syntax.c    — test() (syntax) + testAst() (struktur AST)
 *   test_ir.c        — testIR() + testIRExec()
 *   test_exec.c      — testExec() (execution/semantic/stress)
 *   test_repl.c      — testRepl() (REPL boundary, shared env)
 *   test_fmt.c       — testFmt() (source vs hasil formatter)
 *   test_codegen.c   — testCodegen() (backend C: compile + banding output)
 *   test_dispatch.c  — testDispatch() + tabel kategori + parser argumen
 *
 * Deklarasi publik runner tetap di lib/prompt/prompt.h.
 */

/* test_common.c — cetak source dengan nomor baris di kolom kiri. */
void testPrintSource(const char *src);

/* test_common.c — jalankan lex + parse satu file; true bila AST valid.
 * outBuf/outTokens/outNode merujuk memori milik state global (state
 * dipakai ulang antar file — jangan di-free oleh pemanggil). */
bool testLexParse(State *state, const char *path, Buffer **outBuf, Token **outTokens,
                  Node **outNode);

/* test_codegen.c — compile file via backend C lalu bandingkan output
 * binary hasil compile dengan eksekusi interpreter pada file yang sama. */
void testCodegen(const char *paths[], int length);

#endif /* RUPA_PROMPT_TEST_INTERNAL_H */
