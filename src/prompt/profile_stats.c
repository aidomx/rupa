#include <rupa.h>
#include "profile.h"

/* ================================================================
 * profile_stats.c — mode `rupa profile --cache`
 *
 * Melaporkan kondisi cache in-process saat ini:
 *   - pipeline (AST + IR per file, kunci path + mtime + size)
 *   - canonical pool (subtree AST kanonik lintas file)
 *   - memo parse per-Request (syntaxMemo*)
 *   - FileCache (isi file, prompt/runner.c)
 *   - module cache (hasil load module import)
 *
 * Semua cache hanya hidup selama proses, jadi laporan ini selalu
 * "masih kosong" pada proses CLI tunggal yang baru mulai. Nilai
 * lengkap terlihat dalam proses berumur panjang (REPL, dev server,
 * test multi-grup) — mode ini tetap dijalankan dari CLI untuk audit
 * bentuk laporan, dan jadi dasar mode --clean.
 * ================================================================ */

static void printStatLine(const char *label, int hits, int misses) {
  int total = hits + misses;
  double rate = total > 0 ? (double)hits / (double)total * 100.0 : 0.0;
  printf("  %-22s hits %4d  misses %4d  rate %5.1f%%\n", label, hits, misses, rate);
}

int profilePrintCache(void) {
  printf("Rupa Cache Report v1.0\n\n");
  printf("Cache in-process (pipeline, canonical, FileCache, module)\n");
  printf("hanya hidup selama proses berjalan.\n\n");

  /* --- Pipeline cache: AST + IR per file --- */
  int pipelineCount = pipelineCacheEntryCount();
  printf("pipeline cache: %d entri\n", pipelineCount);
  for (int i = 0; i < pipelineCount; i++) {
    const char *path = pipelineCacheEntryPath(i);
    bool valid = pipelineCacheEntryValid(i);
    printf("  [%d] %s  (%s)\n", i, path ? path : "-", valid ? "valid" : "basi");
  }
  if (pipelineCount == 0)
    printf("  (kosong)\n");
  printStatLine("pipeline get()", pipelineCacheHits(), pipelineCacheMisses());

  /* --- Pool subtree AST kanonik lintas file --- */
  printf("\ncanonical pool : %d entri\n", syntaxCanonicalCount());
  printStatLine("canonical adopt", syntaxCanonicalHits(), syntaxCanonicalMisses());

  /* --- Memo parse per-Request --- */
  printStatLine("memo parse", syntaxMemoHits(), syntaxMemoMisses());

  /* --- FileCache (isi file) --- */
  int fileCount = runnerFileCacheCount();
  printf("\nfile cache     : %d entri\n", fileCount);
  for (int i = 0; i < fileCount; i++) {
    const char *path = runnerFileCachePath(i);
    bool valid = runnerFileCacheValid(i);
    printf("  [%d] %s  (%s)\n", i, path ? path : "-", valid ? "valid" : "basi");
  }
  if (fileCount == 0)
    printf("  (kosong)\n");

  /* --- Module cache (hasil import) --- */
  printf("\nmodule cache   : %d entri\n", moduleCacheCount());

  return 0;
}
