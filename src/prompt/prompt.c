#include <rupa.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

/* Help per-section dari src/prompt/cmd.txt (sumber tunggal).
 * Format: ---name ... ---endname. Loader mencari file di:
 *   1. $RUPA_CMD
 *   2. ./src/prompt/cmd.txt
 *   3. <exe>/../src/prompt/cmd.txt   (run dev dari bin/)
 *   4. <exe>/cmd.txt                 (terpasang di samping binary)
 *   5. <exe>/../share/rupa/cmd.txt
 */

#define CMD_TXT_MAX (64 * 1024)

static char *cmdReadFile(const char *path, char *buf, size_t n) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  size_t got = fread(buf, 1, n - 1, fp);
  fclose(fp);
  buf[got] = '\0';
  return strstr(buf, "---") ? buf : NULL;
}

static bool cmdExeDir(char *buf, size_t n) {
#if defined(__linux__)
  ssize_t len = readlink("/proc/self/exe", buf, n - 1);
  if (len <= 0) return false;
  buf[len] = '\0';
#elif defined(__APPLE__)
  uint32_t size = (uint32_t)n;
  if (_NSGetExecutablePath(buf, &size) != 0) return false;
#else
  (void)n;
  return false;
#endif
  char *slash = strrchr(buf, '/');
  if (!slash) return false;
  *slash = '\0';
  return true;
}

static bool cmdLoadText(char *buf, size_t n) {
  const char *env = getenv("RUPA_CMD");
  if (env && cmdReadFile(env, buf, n)) return true;
  if (cmdReadFile("src/prompt/cmd.txt", buf, n)) return true;
  char base[1024];
  if (cmdExeDir(base, sizeof(base))) {
    char path[1200];
    snprintf(path, sizeof(path), "%s/../src/prompt/cmd.txt", base);
    if (cmdReadFile(path, buf, n)) return true;
    snprintf(path, sizeof(path), "%s/cmd.txt", base);
    if (cmdReadFile(path, buf, n)) return true;
    snprintf(path, sizeof(path), "%s/../share/rupa/cmd.txt", base);
    if (cmdReadFile(path, buf, n)) return true;
  }
  return false;
}

/* Cari section ---<name> ... ---end<name> di text. */
static bool cmdExtractSection(char *text, const char *name, const char **out,
                              size_t *outLen) {
  char *p = text;
  char *start = NULL;
  bool open = false;
  while (*p) {
    char *lineEnd = strchr(p, '\n');
    size_t lineLen = lineEnd ? (size_t)(lineEnd - p) : strlen(p);
    bool isMarker = lineLen >= 3 && strncmp(p, "---", 3) == 0;
    if (isMarker && lineLen > 6 && strncmp(p, "---end", 6) == 0) {
      if (open) {
        size_t content = (size_t)(p - start);
        if (content > 0 && start[content - 1] == '\n') content--;
        *out = start;
        *outLen = content;
        return true;
      }
    } else if (isMarker && !open) {
      size_t nameLen = lineLen - 3;
      if (nameLen == strlen(name) && strncmp(p + 3, name, nameLen) == 0) {
        open = true;
        start = lineEnd ? lineEnd + 1 : p + lineLen;
      }
    }
    p = lineEnd ? lineEnd + 1 : p + lineLen;
  }
  return false;
}

static bool helpPrintSection(const char *name) {
  static char text[CMD_TXT_MAX];
  if (!cmdLoadText(text, sizeof(text))) {
    fprintf(stderr, "help: cmd.txt tidak ditemukan (set $RUPA_CMD atau jalankan dari repo)\n");
    return false;
  }
  const char *content = NULL;
  size_t len = 0;
  if (!cmdExtractSection(text, name, &content, &len)) {
    return false;
  }
  fwrite(content, 1, len, stdout);
  if (len == 0 || content[len - 1] != '\n') printf("\n");
  return true;
}

bool helpShowSection(const char *name) {
  if (!name) return false;
  /* Alias topic lama -> nama section. */
  if (strcmp(name, "fmt") == 0) name = "formatter";
  return helpPrintSection(name);
}

void welcomeMessage() {
  printf("Welcome to Rupa ");
  printf("Rupa v%s", RUPA_VERSION);
  printf("\nPlease .help for more information.\n");
}

void showCliHelp() {
  if (!helpPrintSection("help")) {
    fprintf(stderr, "Usage: rupa --help | help <topic>\n");
  }
}

void showModuleHelp() {
  helpPrintSection("module");
}

void showFmtHelp() {
  helpPrintSection("formatter");
}

void showTestHelp() {
  helpPrintSection("test");
}

void showReplHelp() {
  helpPrintSection("repl");
}

void help(bool is_repl_mode) {
  if (is_repl_mode) {
    showReplHelp();
  } else {
    showCliHelp();
  }
}

void version() {
  printf("Rupa v%s", RUPA_VERSION);
  printf("\nA general-purpose programming language\n");
}
