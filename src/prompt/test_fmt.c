#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * Formatter test: show original source vs formatted output per file.
 * ================================================================ */

void testFmt(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  int modified = 0;
  int unchanged = 0;
  int failed = 0;

  printf("> Formatter tests\n");

  for (int i = 0; i < length; i++) {
    char *source = fmtReadSource(paths[i]);

    /* fmtRunFile owns its GC cleanup; reinitialize before each file
     * when invoked repeatedly in batch mode (see fmtRunPaths). */
    gcinit(100);

    char *formatted = NULL;
    size_t formattedLen = 0;
    FILE *fp = open_memstream(&formatted, &formattedLen);
    int r = fp ? fmtRunFile(paths[i], fp) : 1;
    if (fp) fclose(fp);

    printf("\n--- %s ---\n", paths[i]);
    if (r != 0 || !formatted) {
      printf("FAILED | %s\n", paths[i]);
      failed++;
      free(source);
      free(formatted);
      continue;
    }

    printf("Source:\n");
    if (source && *source) {
      fputs(source, stdout);
      if (source[strlen(source) - 1] != '\n') printf("\n");
    } else {
      printf("(empty)\n");
    }

    bool changed =
        !source || strlen(source) != formattedLen || memcmp(source, formatted, formattedLen) != 0;

    printf("%s:\n", changed ? "Formatted" : "Formatted (unchanged)");
    fwrite(formatted, 1, formattedLen, stdout);
    if (formattedLen == 0 || formatted[formattedLen - 1] != '\n') printf("\n");
    printf("STATUS | %s\n", changed ? "MODIFIED" : "UNCHANGED");

    if (changed)
      modified++;
    else
      unchanged++;
    free(source);
    free(formatted);
  }

  printf("\n> Formatter test summary\n");
  printf("Modified : %d\n", modified);
  printf("Unchanged : %d\n", unchanged);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}
