#include <rupa.h>

/**
 * @brief `rupa spec ...` — konfigurasi proyek (.spec) + enkripsi.
 *
 *   $ rupa spec web | android | software | ios
 *     -> generate .spec.example (plaintext, boleh di-upload)
 *
 *   $ rupa spec -e [sha512|sha256]
 *     -> baca .spec.example, prompt password (hidden, 2x konfirmasi),
 *        tulis .spec terenkripsi. Default algo sha512.
 *
 * `.spec.example` aman di-upload; `.spec` berisi nilai asli (database,
 * host, dsb.) — sebaiknya dienkripsi sebelum ikut repo, kecuali user
 * memang sengaja. Enkripsi: key = hash(password), keystream counter-mode
 * (hash(keyhex || counter)), output hex + header `rupa-spec-enc: <algo>`.
 * `rupa go` membaca .spec terenkripsi dengan prompt password yang sama.
 */

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <openssl/evp.h>
#include <termios.h>
#define SPEC_CRYPTO_AVAILABLE 1
#else
#define SPEC_CRYPTO_AVAILABLE 0
#endif

/* ==================== password prompt (hidden) ==================== */

bool specPromptPassword(const char *prompt, char *buf, int n) {
  if (!buf || n <= 0) return false;
  printf("%s", prompt);
  fflush(stdout);
#if SPEC_CRYPTO_AVAILABLE
  struct termios oldt, tmp;
  if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
    tmp = oldt;
    tmp.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
    bool ok = fgets(buf, n, stdin) != NULL;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
    if (!ok) return false;
    buf[strcspn(buf, "\n")] = '\0';
    return true;
  }
#endif
  /* Bukan tty (pipe/script): baca biasa. */
  if (!fgets(buf, n, stdin)) return false;
  buf[strcspn(buf, "\n")] = '\0';
  return true;
}

/* ==================== crypto helpers ==================== */

#if SPEC_CRYPTO_AVAILABLE
static const EVP_MD *specMd(const char *algo) {
  if (!strcmp(algo, "sha256")) return EVP_sha256();
  if (!strcmp(algo, "sha512")) return EVP_sha512();
  return NULL;
}

static bool specDigestRaw(const char *algo, const unsigned char *data, size_t len,
                          unsigned char *out, unsigned int *outLen) {
  const EVP_MD *md = specMd(algo);
  if (!md) return false;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) return false;
  bool ok = EVP_DigestInit_ex(ctx, md, NULL) == 1 &&
            EVP_DigestUpdate(ctx, data, len) == 1 &&
            EVP_DigestFinal_ex(ctx, out, outLen) == 1;
  EVP_MD_CTX_free(ctx);
  return ok;
}

static void specToHex(const unsigned char *bytes, int len, char *hex) {
  for (int i = 0; i < len; i++)
    sprintf(hex + i * 2, "%02x", bytes[i]);
  hex[len * 2] = '\0';
}

static int specHexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/* Keystream counter-mode: block_c = H(keyhex || ':' || c). */
static bool specKeystream(const char *algo, const char *keyhex, unsigned char *buf,
                          size_t need) {
  char msg[512];
  unsigned char md[EVP_MAX_MD_SIZE];
  unsigned int mdl = 0;
  size_t have = 0;
  unsigned long counter = 0;
  while (have < need) {
    snprintf(msg, sizeof(msg), "%s:%lu", keyhex, counter++);
    if (!specDigestRaw(algo, (const unsigned char *)msg, strlen(msg), md, &mdl))
      return false;
    size_t take = (size_t)mdl < (need - have) ? (size_t)mdl : (need - have);
    memcpy(buf + have, md, take);
    have += take;
  }
  return true;
}
#endif

char *specDecryptText(const char *algo, const char *pass, const char *checkHex,
                      const char *hex) {
#if SPEC_CRYPTO_AVAILABLE
  if (!algo || !pass || !hex) return NULL;
  const EVP_MD *md = specMd(algo);
  if (!md) return NULL;

  size_t hexLen = strlen(hex);
  while (hexLen > 0 && isspace((unsigned char)hex[hexLen - 1])) hexLen--;
  if (hexLen == 0 || hexLen % 2 != 0) return NULL;
  size_t len = hexLen / 2;

  unsigned char *cipher = malloc(len);
  char *plain = malloc(len + 1);
  unsigned char *stream = malloc(len);
  unsigned char key[EVP_MAX_MD_SIZE];
  unsigned char mdBuf[EVP_MAX_MD_SIZE];
  unsigned int keyLen = 0, mdLen = 0;
  char *keyhex = malloc(EVP_MAX_MD_SIZE * 2 + 1);
  char plainHex[EVP_MAX_MD_SIZE * 2 + 1];
  if (!cipher || !plain || !stream || !keyhex) {
    free(cipher); free(plain); free(stream); free(keyhex);
    return NULL;
  }

  for (size_t i = 0; i < len; i++) {
    int hi = specHexVal(hex[i * 2]), lo = specHexVal(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) {
      free(cipher); free(plain); free(stream); free(keyhex);
      return NULL;
    }
    cipher[i] = (unsigned char)((hi << 4) | lo);
  }

  char msg[512];
  char *result = NULL;
  for (int attempt = 0; attempt < 2 && !result; attempt++) {
    if (attempt == 0) {
      /* Percobaan 1: password persis. Percobaan 2 (compat): password
       * yang sudah di-trim trailing whitespace. */
      snprintf(msg, sizeof(msg), "%s", pass);
    } else {
      snprintf(msg, sizeof(msg), "%s", pass);
      char *end = msg + strlen(msg);
      while (end > msg && isspace((unsigned char)end[-1])) *--end = '\0';
      if (!strcmp(msg, pass)) break; /* sama — tak ada varian kedua */
    }
    if (!specDigestRaw(algo, (const unsigned char *)msg, strlen(msg), key, &keyLen))
      break;
    specToHex(key, (int)keyLen, keyhex);
    if (!specKeystream(algo, keyhex, stream, len))
      break;
    for (size_t i = 0; i < len; i++)
      plain[i] = (char)(cipher[i] ^ stream[i]);
    plain[len] = '\0';

    /* Verifikasi: digest(plain) harus cocok dengan check di header
     * (16 hex pertama, disimpan saat encrypt). Password salah =
     * plaintext sampah = digest tak mungkin cocok. */
    if (!checkHex || strlen(checkHex) < 16) break;
    if (!specDigestRaw(algo, (const unsigned char *)plain, len, mdBuf, &mdLen))
      break;
    specToHex(mdBuf, (int)mdLen, plainHex);
    if (strncmp(plainHex, checkHex, 16) != 0)
      continue;
    result = plain;
  }
  free(cipher); free(stream); free(keyhex);
  if (!result) {
    memset(plain, 0, len + 1);
    free(plain);
    (void)md;
    return NULL;
  }
  return result;
#else
  (void)algo; (void)pass; (void)checkHex; (void)hex;
  return NULL;
#endif
}

/* ==================== .spec.example templates ==================== */

static const char *specExampleWeb =
    "name: your app name\n"
    "author: -\n"
    "version: \"1.0\"\n"
    "\n"
    "target: web\n"
    "arch: unknown\n"
    "debug: true\n"
    "release: false\n"
    "domain: example.com\n"
    "settings:\n"
    "  - host: 127.0.0.1\n"
    "  - port: 8000\n"
    "  - protocol: http\n"
    "  - dbhost: 127.0.0.1\n"
    "  - dbport: 5432\n"
    "  - dbname: app\n"
    "  - dbuser: -\n"
    "  - dbpass: -\n"
    "  - vps:\n"
    "    - settingan vps\n";

static const char *specExampleGeneric =
    "name: your app name\n"
    "author: -\n"
    "version: \"1.0\"\n"
    "\n"
    "target: TARGET\n"
    "arch: unknown\n"
    "debug: false\n"
    "release: false\n"
    "settings:\n"
    "  - dbhost: 127.0.0.1\n"
    "  - dbport: 5432\n"
    "  - dbname: app\n"
    "  - dbuser: -\n"
    "  - dbpass: -\n";

static bool specFileExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static bool specWriteExample(const char *target) {
  if (specFileExists(".spec.example")) {
    fprintf(stderr, "rupa spec: .spec.example sudah ada — hapus dulu bila ingin regenerasi\n");
    return false;
  }
  const char *body = !strcmp(target, "web")
                         ? specExampleWeb
                         : specExampleGeneric;
  FILE *fp = fopen(".spec.example", "wb");
  if (!fp) {
    fprintf(stderr, "rupa spec: cannot write .spec.example\n");
    return false;
  }
  for (const char *p = body; *p; p++) {
    const char *rep = NULL;
    if (!strncmp(p, "TARGET", 6)) rep = target;
    if (rep) {
      fputs(rep, fp);
      p += 5;
    } else {
      fputc(*p, fp);
    }
  }
  fclose(fp);
  printf("rupa spec: generated .spec.example (target=%s)\n", target);
  printf("rupa spec: isi nilai asli di sini, lalu 'rupa spec -e' untuk mengenkripsi menjadi .spec\n");
  return true;
}

/* ==================== encrypt ==================== */

static char *specReadAll(const char *path) {
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

static bool specEncrypt(const char *algo) {
#if !SPEC_CRYPTO_AVAILABLE
  (void)algo;
  fprintf(stderr, "rupa spec: enkripsi tidak tersedia di platform ini\n");
  return false;
#else
  if (!specMd(algo)) {
    fprintf(stderr, "rupa spec: unknown algo '%s' (sha512, sha256)\n", algo);
    return false;
  }
  char *plain = specReadAll(".spec.example");
  if (!plain) {
    fprintf(stderr, "rupa spec: .spec.example tidak ada — jalankan 'rupa spec <target>' dulu\n");
    return false;
  }

  char pass[256], confirm[256];
  if (!specPromptPassword("spec password: ", pass, sizeof(pass)) || !*pass) {
    fprintf(stderr, "rupa spec: password kosong\n");
    free(plain);
    return false;
  }
  if (!specPromptPassword("confirm password: ", confirm, sizeof(confirm)) ||
      strcmp(pass, confirm) != 0) {
    fprintf(stderr, "rupa spec: password tidak cocok\n");
    memset(pass, 0, sizeof(pass));
    memset(confirm, 0, sizeof(confirm));
    free(plain);
    return false;
  }

  /* key = H(password); keystream counter-mode; XOR; output hex. */
  unsigned char key[EVP_MAX_MD_SIZE];
  unsigned int keyLen = 0;
  bool ok = specDigestRaw(algo, (const unsigned char *)pass, strlen(pass), key, &keyLen);
  memset(pass, 0, sizeof(pass));
  memset(confirm, 0, sizeof(confirm));
  if (!ok) {
    fprintf(stderr, "rupa spec: digest gagal\n");
    free(plain);
    return false;
  }
  char *keyhex = malloc(EVP_MAX_MD_SIZE * 2 + 1);
  if (!keyhex) {
    free(plain);
    return false;
  }
  specToHex(key, (int)keyLen, keyhex);

  size_t len = strlen(plain);
  unsigned char *stream = malloc(len ? len : 1);
  char *hex = malloc(len * 2 + 1);
  if (!stream || !hex) {
    free(keyhex); free(stream); free(hex); free(plain);
    return false;
  }
  ok = specKeystream(algo, keyhex, stream, len);
  free(keyhex);
  if (!ok) {
    fprintf(stderr, "rupa spec: keystream gagal\n");
    free(stream); free(hex); free(plain);
    return false;
  }
  /* Check = 16 hex pertama dari digest PLAINTEXT — dihitung SEBELUM
   * di-XOR (buffer plain masih teks asli di titik ini); diverifikasi
   * saat decrypt (password salah tertangkap deterministik). */
  unsigned char chk[EVP_MAX_MD_SIZE];
  unsigned int chkLen = 0;
  char chkHex[EVP_MAX_MD_SIZE * 2 + 1];
  if (!specDigestRaw(algo, (const unsigned char *)plain, len, chk, &chkLen)) {
    fprintf(stderr, "rupa spec: digest gagal\n");
    free(hex); free(plain); free(stream);
    return false;
  }
  specToHex(chk, (int)chkLen, chkHex);

  for (size_t i = 0; i < len; i++)
    plain[i] = (char)(plain[i] ^ stream[i]);
  free(stream);
  specToHex((const unsigned char *)plain, (int)len, hex);
  memset(plain, 0, len);
  free(plain);

  FILE *fp = fopen(".spec", "wb");
  if (!fp) {
    fprintf(stderr, "rupa spec: cannot write .spec\n");
    free(hex);
    return false;
  }
  fprintf(fp, "rupa-spec-enc: %s\ncheck: %.16s\n%s\n", algo, chkHex, hex);
  fclose(fp);
  free(hex);

  printf("rupa spec: .spec terenkripsi (%s)\n", algo);
  printf("rupa spec: .spec.example tetap plaintext — keduanya boleh di-upload;\n");
  printf("rupa spec: bila .spec berisi kredensial asli (database), enkripsi dulu sebelum upload\n");
  return true;
#endif
}

/* ==================== decrypt (spec -d) ==================== */

/* Baca .spec terenkripsi, prompt password sekali, tulis kembali
 * .spec.example (plaintext). Menimpa hanya dengan konfirmasi. */
static bool specDecrypt(void) {
#if !SPEC_CRYPTO_AVAILABLE
  fprintf(stderr, "rupa spec: dekripsi tidak tersedia di platform ini\n");
  return false;
#else
  char *content = specReadAll(".spec");
  if (!content) {
    fprintf(stderr, "rupa spec: .spec tidak ada\n");
    return false;
  }
  if (strncmp(content, "rupa-spec-enc:", 14) != 0) {
    fprintf(stderr, "rupa spec: .spec bukan output 'rupa spec -e' (tanpa header)\n");
    free(content);
    return false;
  }
  char algo[32] = {0};
  if (sscanf(content + 14, "%31s", algo) != 1 || !specMd(algo)) {
    fprintf(stderr, "rupa spec: header algo tidak valid\n");
    free(content);
    return false;
  }
  const char *cur = strchr(content, '\n');
  if (!cur) {
    fprintf(stderr, "rupa spec: .spec tidak berisi payload\n");
    free(content);
    return false;
  }
  cur++;
  const char *checkHex = NULL;
  if (!strncmp(cur, "check: ", 7)) {
    checkHex = cur + 7;
    cur = strchr(cur, '\n');
    if (!cur) {
      fprintf(stderr, "rupa spec: .spec tidak berisi payload\n");
      free(content);
      return false;
    }
    cur++;
  }
  if (!checkHex) {
    fprintf(stderr,
            "rupa spec: header 'check:' tidak ada — regenerate dengan 'rupa spec -e'\n");
    free(content);
    return false;
  }

  char pass[256];
  if (!specPromptPassword("spec password: ", pass, sizeof(pass)) || !*pass) {
    fprintf(stderr, "rupa spec: password kosong\n");
    free(content);
    return false;
  }
  char *plain = specDecryptText(algo, pass, checkHex, cur);
  memset(pass, 0, sizeof(pass));
  free(content);
  if (!plain) {
    fprintf(stderr, "rupa spec: decrypt gagal (password salah?)\n");
    return false;
  }

  /* .spec.example sudah ada -> tulis .spec.example.out saja, biar user
   * yang memutuskan menimpa. */
  const char *outPath = ".spec.example";
  if (specFileExists(outPath)) outPath = ".spec.example.out";
  FILE *fp = fopen(outPath, "wb");
  if (!fp) {
    fprintf(stderr, "rupa spec: cannot write %s\n", outPath);
    free(plain);
    return false;
  }
  fputs(plain, fp);
  fclose(fp);
  memset(plain, 0, strlen(plain));
  free(plain);
  printf("rupa spec: decrypted -> %s (%s)\n", outPath, algo);
  return true;
#endif
}

/* ==================== command ==================== */

static void specHelp(void) {
  printf("Usage: rupa spec <target> | rupa spec -e [algo] | rupa spec -d\n\n");
  printf("  spec <target>        generate .spec.example (web, android, software, ios)\n");
  printf("  spec -e [algo]       enkripsi .spec.example menjadi .spec\n");
  printf("                       (default algo sha512; prompt password hidden)\n");
  printf("  spec -d              decrypt .spec kembali ke .spec.example\n");
  printf("                       (prompt password hidden)\n\n");
  printf(".spec.example: plaintext, aman di-upload.\n");
  printf(".spec        : versi terenkripsi — aman untuk kredensial (database, host);\n");
  printf("               upload bentuk encrypted atau tidak, terserah Anda.\n");
  printf("'rupa go' membaca .spec terenkripsi dengan prompt password.\n");
}

int specCommand(const char *paths[], int length) {
  if (!paths || length <= 0) {
    specHelp();
    return 0;
  }
  const char *a = paths[0];
  if (!strcmp(a, "-h") || !strcmp(a, "--help") || !strcmp(a, "help")) {
    specHelp();
    return 0;
  }
  if (!strcmp(a, "-e")) {
    const char *algo = "sha512";
    if (length > 1 && paths[1][0] != '-') {
      algo = paths[1];
    } else if (length > 1 && paths[1][0] == '-') {
      fprintf(stderr, "rupa spec: -e accepts an optional algo (sha512, sha256)\n");
      return 1;
    }
    return specEncrypt(algo) ? 0 : 1;
  }
  if (!strcmp(a, "-d")) return specDecrypt() ? 0 : 1;
  /* Target generate: web/android/software/ios. */
  if (!strcmp(a, "web") || !strcmp(a, "android") || !strcmp(a, "software") ||
      !strcmp(a, "ios"))
    return specWriteExample(a) ? 0 : 1;

  fprintf(stderr, "rupa spec: unknown subcommand '%s'\n", a);
  specHelp();
  return 1;
}
