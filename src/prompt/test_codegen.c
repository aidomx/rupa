#include <rupa.h>
#include "test_internal.h"

/* ================================================================
 * test_codegen.c — backend C test (`rupa test codegen`)
 *
 * Regression untuk jalur `rupa <file> -o` (codegen/c.c): setiap file
 * di tests/codegen/*.rp di-compile ke binary kecil via compileIR()
 * lalu binary dijalankan. Output stdout binary dibandingkan dengan
 * output eksekusi interpreter (run()) pada file yang sama — keduanya
 * HARUS identik (backend C wajib meniru semantik IR machine).
 *
 * File test di folder ini sengaja hanya memakai konstruksi yang
 * didukung backend C (tanpa IR_INTERP murni/async) — konstruksi di
 * luar cakupan backend tetap dites di kategori lain.
 * ================================================================ */

#define CG_BIN_MAX 1024

/* Jalankan command dengan stdout dialihkan ke outPath. Return exit
 * status (0 = sukses). stderr dibiarkan tampil — error runtime harus
 * terlihat saat debug. */
static int cgRunCapture(const char *cmd, const char *outPath) {
  char full[CG_BIN_MAX + 256];
  snprintf(full, sizeof(full), "%s > %s", cmd, outPath);
  int status = system(full);
  return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

/* Baca seluruh file ke buffer malloc'd (NULL bila gagal). */
static char *cgReadFile(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }
  char *buf = malloc((size_t)size + 1);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  size_t got = fread(buf, 1, (size_t)size, fp);
  fclose(fp);
  buf[got] = '\0';
  return buf;
}

void testCodegen(const char *paths[], int length) {
  if (!paths || length <= 0) {
    printf("No test files.\n");
    return;
  }

  State *state = createGlobalState(1, false);
  if (!state || !state->buffer) {
    fprintf(stderr, "Failed to create test state.\n");
    return;
  }

  int passed = 0, failed = 0;
  printf("> Codegen (C backend) tests\n");

  for (int i = 0; i < length; i++) {
    const char *path = paths[i];
    printf("\n--- %s ---\n", path);

    /* Nama unik binary per file: /tmp/rupa_cg_<hash>. */
    unsigned long h = 1469598103934665603ULL;
    for (const char *p = path; *p; p++) {
      h = (h ^ (unsigned char)*p) * 1099511628211ULL;
    }
    char binPath[64], interpOut[80], codegenOut[80];
    snprintf(binPath, sizeof(binPath), "/tmp/rupa_cg_%lx", h);
    snprintf(interpOut, sizeof(interpOut), "%s.interp", binPath);
    snprintf(codegenOut, sizeof(codegenOut), "%s.codegen", binPath);

    /* 1. Compile file -> binary via backend C (compileIR menghapus
     *    source C sementara sendiri). */
    Node *node = NULL;
    IRModule *ir = NULL;
    if (!pipelineCacheGet(path, &node, &ir)) {
      Buffer *buffer;
      Token *tokens;
      if (!testLexParse(state, path, &buffer, &tokens, &node)) {
        printf("FAIL | %s (lex/parse failed)\n", path);
        failed++;
        continue;
      }
      pipelineCachePut(path, node, NULL);
    }
    if (!ir) {
      ir = createIR();
      if (!ir || !rewrite(node, -1, ir)) {
        printf("FAIL | %s (rewrite failed)\n", path);
        failed++;
        continue;
      }
      pipelineCachePut(path, node, ir);
    }

    bool compileOk = (compileIR(ir, binPath) == 0) && access(binPath, X_OK) == 0;
    if (!compileOk) {
      printf("FAIL | %s (compile failed — cek opcode yang belum ditangani backend)\n", path);
      failed++;
      continue;
    }

    /* 2. Jalankan binary hasil compile, tangkap stdout. */
    char binCmd[CG_BIN_MAX];
    snprintf(binCmd, sizeof(binCmd), "%s", binPath);
    int binStatus = cgRunCapture(binCmd, codegenOut);

    /* 3. Jalankan interpreter (run() = jalur `rupa file`), tangkap
     *    stdout. run() menerima bentuk argv lengkap. */
    const char *runPaths[2] = {"rupa", path};
    int interpStatus = cgRunCapture("true", interpOut); /* placeholder sink */
    (void)interpStatus;
    runnerCaptureStdoutTo(interpOut, runPaths, 2);
    /* Status interpreter tidak dibandingkan langsung — perbedaan status
     * terlihat dari error di stderr; yang dibandingkan = stdout. */

    /* 4. Bandingkan output. */
    char *outBin = cgReadFile(codegenOut);
    char *outInterp = cgReadFile(interpOut);

    bool same = outBin && outInterp && strcmp(outBin, outInterp) == 0;
    if (same && binStatus != 0) {
      /* Binary gagal saat runtime — hanya sah bila interpreter juga
       * error (dua-duanya gagal). Output bisa saja sama (kosong), tapi
       * program error harus terdeteksi. Interpreter status tidak
       * tersedia murah di sini; anggap gagal bila binary error. */
      printf("FAIL | %s (compiled binary exit=%d — runtime error)\n", path, binStatus);
      failed++;
    } else if (!same) {
      printf("FAIL | %s (output berbeda — backend C menyimpang dari interpreter)\n", path);
      printf("       interpreter : [%s]\n", outInterp ? outInterp : "(null)");
      printf("       codegen     : [%s]\n", outBin ? outBin : "(null)");
      failed++;
    } else {
      printf("PASS | %s\n", path);
      passed++;
    }

    free(outBin);
    free(outInterp);
    unlink(binPath);
    unlink(interpOut);
    unlink(codegenOut);
  }

  printf("\n> Codegen test summary\n");
  printf("Passed : %d\n", passed);
  printf("Failed : %d\n", failed);
  printf("Status : %s\n", failed == 0 ? "Success" : "Failed");
}
