#include <rupa.h>
#include "profile.h"

/* ================================================================
 * profile_clean.c — mode `rupa profile --clean`
 *
 * Mengosongkan seluruh cache in-process rupa. Karena cache hanya
 * hidup selama proses, pembersihan yang berguna adalah saat cache
 * dipakai dalam proses berumur panjang (REPL, dev server, test
 * multi-grup) — di command line tunggal proses selesai seketika.
 * Mode ini tetap dijalankan dari CLI sebagai titik audit tujuan
 * bersama, dan dipakai unit lain (mis. dev server) sebelum reload.
 * ================================================================ */

int profileClean(void) {
  /* Urutan bebas; reset hanya melupakan pointer (GC arena melepas
   * memori di gcclean) — pipeline.h mewajibkan reset SEBELUM gcclean.
   * loader.c sudah memanggil gcclean di ujung dispatch `profile`. */
  pipelineCacheReset();
  syntaxCanonicalReset();

  runnerFileCacheReset();
  moduleCacheReset();

  printf("Cache dibersihkan:\n");
  printf("  - pipeline cache (AST + IR) : kosong\n");
  printf("  - canonical pool            : kosong\n");
  printf("  - file cache                : kosong\n");
  printf("  - module cache              : kosong\n");
  return 0;
}
