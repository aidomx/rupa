#ifndef RUPA_PROMPT_PROFILE_H
#define RUPA_PROMPT_PROFILE_H

#include <rupa.h>

/*
 * profile.h — kontrak internal antar-unit `rupa profile` (rules.md:
 * modular, jangan membengkak). runner.c masih menampung inti profileRun
 * (pengukuran dua fase + profilePrint); unit pendukung dipisah:
 *
 *   profile_stats.c     — profilePrintCache (laporan daftar cache)
 *   profile_clean.c     — profileClean (reset semua cache in-process)
 *   profile_optimizer.c — profileOptimizer (deteksi duplikasi lintas file
 *                         via pool subtree AST kanonik + saran patch)
 *
 * Deklarasi publik profileRun tetap di lib/prompt/prompt.h.
 */

/* profile_stats.c — cetak daftar cache in-process beserta statistik
 * hit/miss dan status kegiatan (valid/basi). Return exit code. */
int profilePrintCache(void);

/* profile_clean.c — kosongkan seluruh cache in-process (pipeline AST+IR,
 * pool kanonik, FileCache, module cache). Return exit code. */
int profileClean(void);

/* profile_optimizer.c — pindai file .rp di bawah root (rekursif), lalu
 * parse satu per satu: ekspresi terluar yang identik antar file masuk
 * pool kanonik (syntaxCanonical*) dan tercatat sebagai adopt (duplikasi).
 * Laporan: file + baris + sumber + berapa kali muncul, diikuti saran
 * patch (ekstraksi helper). Return exit code. */
int profileOptimizer(const char *root);

/* runner.c — daftar & reset FileCache (cache isi file, diukur via
 * mtime+nsec+size). Dipakai profilePrintCache/profileClean. */
int runnerFileCacheCount(void);
const char *runnerFileCachePath(int index);
bool runnerFileCacheValid(int index);
void runnerFileCacheReset(void);

#endif /* RUPA_PROMPT_PROFILE_H */
